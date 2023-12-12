#ifndef CXKERNEL_CRYPTFILEDEVICE_H
#define CXKERNEL_CRYPTFILEDEVICE_H
#include "cxkernel/cxkernelinterface.h"
#include <QtCore/QIODevice>

#define AES_BLOCK_SIZE 16

class QFileDevice;

struct aes_key_st;
struct CtrState {
  unsigned char ivec[AES_BLOCK_SIZE];
  unsigned int num;
  unsigned char ecount[AES_BLOCK_SIZE];
};

namespace cxkernel {
    class CXKERNEL_API CryptFileDevice : public QIODevice {
        Q_OBJECT
            Q_DISABLE_COPY(CryptFileDevice)
    public:
        enum class AesKeyLength : quint32 {
            kAesKeyLength128,
            kAesKeyLength192,
            kAesKeyLength256
        };

        explicit CryptFileDevice(QObject* parent = nullptr);
        explicit CryptFileDevice(QFileDevice* device, QObject* parent = nullptr);
        explicit CryptFileDevice(QFileDevice* device, const QByteArray& password,
            const QByteArray& salt, QObject* parent = nullptr);
        explicit CryptFileDevice(const QString& fileName, const QByteArray& password,
            const QByteArray& salt, QObject* parent = nullptr);
        ~CryptFileDevice() override;

        bool open(OpenMode flags) override;
        void close() override;

        void setFileName(const QString& fileName);
        QString fileName() const;

        void setFileDevice(QFileDevice* device);

        void setPassword(const QByteArray& password);
        void setSalt(const QByteArray& salt);
        void setKeyLength(AesKeyLength keyLength);
        void setNumRounds(int numRounds);

        bool isEncrypted() const;
        qint64 size() const override;

        bool atEnd() const override;
        qint64 bytesAvailable() const override;
        qint64 pos() const override;
        bool seek(qint64 pos) override;
        bool flush();
        bool remove();
        bool exists() const;
        bool rename(const QString& newName);

    protected:
        qint64 readData(char* data, qint64 len) override;
        qint64 writeData(const char* data, qint64 len) override;

        qint64 readBlock(qint64 len, QByteArray& ba);

    private:
        bool initCipher();
        void initCtr(CtrState* state, const unsigned char* iv);
        char* encrypt(const char* plainText, qint64 len);
        char* decrypt(const char* cipherText, qint64 len);

        void insertHeader();
        bool tryParseHeader();

        QFileDevice* m_device = nullptr;
        bool m_deviceOwner = false;
        bool m_encrypted = false;

        QByteArray m_password;
        QByteArray m_salt;
        AesKeyLength m_aesKeyLength = AesKeyLength::kAesKeyLength256;
        int m_numRounds = 5;

        CtrState m_ctrState = {};
        aes_key_st* m_aesKey;
    };
}
#endif  // CXKERNEL_CRYPTFILEDEVICE_H
