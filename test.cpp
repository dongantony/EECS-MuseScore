// Unit tests using Google Test (gtest) framework
#include <gtest/gtest.h>
#include <QLocalSocket>
#include <QByteArray>
#include <QDataStream>
#include <functional>
#include <iostream>
#include "ipc.h"

// Mock class to simulate QLocalSocket behavior for testing
class MockLocalSocket : public QLocalSocket {
public:
    QByteArray writtenData;
    QByteArray readData;

    qint64 writeBytes(const char *data, qint64 len) override {
        writtenData.append(data, len);
        return len;
    }

    void setReadData(const QByteArray &data) {
        readData = data;
    }

    qint64 bytesAvailable() const override {
        return readData.size();
    }

    bool waitForBytesWritten(int msecs) override {
        Q_UNUSED(msecs);
        return true;
    }

    bool waitForReadyRead(int msecs) override {
        Q_UNUSED(msecs);
        return true;
    }

    qint64 read(char *data, qint64 maxlen) override {
        qint64 len = qMin(maxlen, static_cast<qint64>(readData.size()));
        memcpy(data, readData.constData(), len);
        readData.remove(0, len);
        return len;
    }
};

class IpcTest : public ::testing::Test {
protected:
    MockLocalSocket socket;
};

// Test for writeToSocket
TEST_F(IpcTest, WriteToSocket_ValidData) {
    QByteArray data = "test data";
    bool result = muse::ipc::writeToSocket(&socket, data);

    EXPECT_TRUE(result);
    EXPECT_EQ(socket.writtenData, data);
}

TEST_F(IpcTest, WriteToSocket_DataExceedsMaxPackageSize) {
    QByteArray data(3000, 'a'); // Create a large data array exceeding MAX_PACKAGE_SIZE
    bool result = muse::ipc::writeToSocket(&socket, data);

    EXPECT_FALSE(result);
    EXPECT_TRUE(socket.writtenData.isEmpty());
}

// Test for readFromSocket
TEST_F(IpcTest, ReadFromSocket_ValidData) {
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream << static_cast<quint32>(10) << QByteArray("test data");

    socket.setReadData(data);

    bool result = muse::ipc::readFromSocket(&socket, [](const QByteArray &data) {
        EXPECT_EQ(data, QByteArray("test data"));
    });

    EXPECT_TRUE(result);
}

TEST_F(IpcTest, ReadFromSocket_DataExceedsMaxPackageSize) {
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream << static_cast<quint32>(3000); // Set a size larger than MAX_PACKAGE_SIZE

    socket.setReadData(data);

    bool result = muse::ipc::readFromSocket(&socket, [](const QByteArray &) {
        // Callback should not be called
        ADD_FAILURE() << "Callback should not be called for oversized data";
    });

    EXPECT_FALSE(result);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}