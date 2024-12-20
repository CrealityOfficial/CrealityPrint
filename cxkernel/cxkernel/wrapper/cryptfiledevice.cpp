#include "cryptfiledevice.h"

#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/modes.h>

#include <limits>

#include <QtCore/QtEndian>

#include <QtCore/QDataStream>
#include <QtCore/QFile>
#include <QtCore/QFileDevice>
#include <QtCore/QVector>

#include <QtCore/QCryptographicHash>

// TODO: remove fixed header length in version 2
static int const kHeaderLength = 128;
static int const kPaddingLength = 54;
static int const kHashLength = 32;
static int const kSaltMaxLength = 8;

namespace cxkernel {
    CryptFileDevice::CryptFileDevice(QObject* parent) : QIODevice(parent)
    {
        m_aesKey = new AES_KEY();
    }

    CryptFileDevice::CryptFileDevice(QFileDevice* device, QObject* parent)
        : QIODevice(parent), m_device(device) {
        m_aesKey = new AES_KEY();
    }

    CryptFileDevice::CryptFileDevice(QFileDevice* device,
        const QByteArray& password,
        const QByteArray& salt, QObject* parent)
        : QIODevice(parent), m_device(device), m_password(password) {
        m_aesKey = new AES_KEY();
        setSalt(salt);
    }

    CryptFileDevice::CryptFileDevice(const QString& fileName,
        const QByteArray& password,
        const QByteArray& salt, QObject* parent)
        : QIODevice(parent),
        m_device(new QFile(fileName)),
        m_deviceOwner(true),
        m_password(password) {
        m_aesKey = new AES_KEY();
        setSalt(salt);
    }

    CryptFileDevice::~CryptFileDevice() {
        close();

        if (m_deviceOwner) delete m_device;
    }

    void CryptFileDevice::setPassword(const QByteArray& password) {
        m_password = password;
    }

    void CryptFileDevice::setSalt(const QByteArray& salt) {
        if (salt.isEmpty()) {
            m_salt.clear();
            return;
        }

        m_salt = salt.leftJustified(kSaltMaxLength, '\0', true);
    }

    void CryptFileDevice::setKeyLength(AesKeyLength keyLength) {
        m_aesKeyLength = keyLength;
    }

    void CryptFileDevice::setNumRounds(int numRounds) { m_numRounds = numRounds; }

    bool CryptFileDevice::open(OpenMode mode) {
        if (m_device == nullptr) return false;

        if (isOpen()) return false;

        if (mode & WriteOnly) mode |= ReadOnly;

        if (mode & Append) mode |= ReadWrite;

        OpenMode deviceOpenMode;
        if (mode == ReadOnly)
            deviceOpenMode = ReadOnly;
        else
            deviceOpenMode = ReadWrite;

        if (mode & Truncate) deviceOpenMode |= Truncate;

        bool ok;
        if (m_device->isOpen())
            ok = (m_device->openMode() == deviceOpenMode);
        else
            ok = m_device->open(deviceOpenMode);

        if (!ok) return false;

        if (m_password.isEmpty()) {
            setOpenMode(mode);
            return true;
        }

        if (!initCipher()) return false;

        m_encrypted = true;
        setOpenMode(mode);

        qint64 size = m_device->size();
        if (size == 0 && mode != ReadOnly) insertHeader();

        if (size > 0) {
            if (!tryParseHeader()) {
                m_encrypted = false;
                m_device->seek(0);
                m_device->close();
                return false;
            }
        }

        if (mode & Append) seek(m_device->size() - kHeaderLength);

        return true;
    }

    void CryptFileDevice::insertHeader() {
        QDataStream ostream(m_device);
        ostream << quint8(0xcd);                          // cryptdevice byte
        ostream << quint8(0x01);                          // version
        ostream << static_cast<quint32>(m_aesKeyLength);  // aes key length
        ostream << static_cast<qint32>(m_numRounds);      // iteration count to use
        ostream.writeRawData(
            QCryptographicHash::hash(m_password, QCryptographicHash::Sha3_256),
            kHashLength);
        ostream.writeRawData(
            QCryptographicHash::hash(m_salt, QCryptographicHash::Sha3_256),
            kHashLength);
        ostream.writeRawData(QByteArray(kPaddingLength, char(0xcd)), kPaddingLength);
    }

    bool CryptFileDevice::tryParseHeader() {
        QDataStream istream(m_device);
        quint8 cdByte;
        istream >> cdByte;
        if (cdByte != 0xcd) return false;

        quint8 version;
        istream >> version;
        if (version != 0x01) return false;

        quint32 aesKeyLength;
        istream >> aesKeyLength;
        if (static_cast<AesKeyLength>(aesKeyLength) != m_aesKeyLength) return false;

        qint32 numRounds;
        istream >> numRounds;
        if (numRounds != m_numRounds) return false;

        QByteArray hash(kHashLength, '\0');
        int read = istream.readRawData(hash.data(), kHashLength);
        if (read != kHashLength) return false;

        QByteArray expectedPasswordHash =
            QCryptographicHash::hash(m_password, QCryptographicHash::Sha3_256);
        if (hash != expectedPasswordHash) return false;

        read = istream.readRawData(hash.data(), kHashLength);
        if (read != kHashLength) return false;

        QByteArray expectedSaltHash =
            QCryptographicHash::hash(m_salt, QCryptographicHash::Sha3_256);
        if (hash != expectedSaltHash) return false;

        QByteArray padding(kPaddingLength, '\0');
        read = istream.readRawData(padding.data(), kPaddingLength);
        if (read != kPaddingLength) return false;

        QByteArray expectedPadding(kPaddingLength, char(0xcd));
        return padding == expectedPadding;
    }

    void CryptFileDevice::close() {
        if (!isOpen()) return;

        if ((openMode() & WriteOnly) || (openMode() & Append)) flush();

        seek(0);
        m_device->close();
        setOpenMode(NotOpen);

        if (m_encrypted) m_encrypted = false;
    }

    void CryptFileDevice::setFileName(const QString& fileName) {
        if (m_device) {
            m_device->close();
            if (m_deviceOwner) delete m_device;
        }
        m_device = new QFile(fileName);
        m_deviceOwner = true;
    }

    QString CryptFileDevice::fileName() const {
        if (m_device != nullptr) return m_device->fileName();

        return QString();
    }

    void CryptFileDevice::setFileDevice(QFileDevice* device) {
        if (m_device) {
            m_device->close();
            if (m_deviceOwner) delete m_device;
        }
        m_device = device;
        m_deviceOwner = false;
    }

    bool CryptFileDevice::flush() { return m_device->flush(); }

    bool CryptFileDevice::isEncrypted() const { return m_encrypted; }

    qint64 CryptFileDevice::readBlock(qint64 len, QByteArray& ba) {
        int length = ba.length();
        qint64 readBytes = 0;
        do {
            qint64 fileRead = m_device->read(ba.data() + ba.length(), len - readBytes);
            if (fileRead <= 0) break;

            readBytes += fileRead;
        } while (readBytes < len);

        if (readBytes == 0) return 0;

        QScopedArrayPointer<char> plaintext(decrypt(ba.data() + length, readBytes));

        ba.append(plaintext.data(), readBytes);

        return readBytes;
    }

    qint64 CryptFileDevice::readData(char* data, qint64 len) {
        if (!m_encrypted) {
            qint64 fileRead = m_device->read(data, len);
            return fileRead;
        }

        if (len == 0) return m_device->read(data, len);

        QByteArray ba;
        ba.reserve(len);
        do {
            qint64 maxSize = len - ba.length();

            qint64 size = readBlock(maxSize, ba);

            if (size == 0) break;
        } while (ba.length() < len);

        if (ba.isEmpty()) return 0;

        memcpy(data, ba.data(), ba.length());

        return ba.length();
    }

    qint64 CryptFileDevice::writeData(const char* data, qint64 len) {
        if (!m_encrypted) return m_device->write(data, len);

        QScopedArrayPointer<char> cipherText(encrypt(data, len));
        m_device->write(cipherText.data(), len);

        return len;
    }

    void CryptFileDevice::initCtr(CtrState* state, const unsigned char* iv) {
        qint64 position = pos();

        state->num = position % AES_BLOCK_SIZE;

        memset(state->ecount, 0, sizeof(state->ecount));

        /* Initialise counter in 'ivec' */
        qint64 count = position / AES_BLOCK_SIZE;
        if (state->num > 0) count++;

        qint64 newCount = count;
        if (newCount > 0) newCount = qToBigEndian(count);

        int sizeOfIv = sizeof(state->ivec) - sizeof(qint64);
        memcpy(state->ivec + sizeOfIv, &newCount, sizeof(newCount));

        /* Copy IV into 'ivec' */
        memcpy(state->ivec, iv, sizeOfIv);

        if (count > 0) {
            count = qToBigEndian(count - 1);
            unsigned char prevIvec[AES_BLOCK_SIZE];
            memcpy(prevIvec, state->ivec, sizeOfIv);

            memcpy(prevIvec + sizeOfIv, &count, sizeof(count));

            AES_encrypt(prevIvec, state->ecount, m_aesKey);
        }
    }

    bool CryptFileDevice::initCipher() {
        const EVP_CIPHER* cipher = EVP_enc_null();
        if (m_aesKeyLength == AesKeyLength::kAesKeyLength128)
            cipher = EVP_aes_128_ctr();
        else if (m_aesKeyLength == AesKeyLength::kAesKeyLength192)
            cipher = EVP_aes_192_ctr();
        else if (m_aesKeyLength == AesKeyLength::kAesKeyLength256)
            cipher = EVP_aes_256_ctr();
        else
            Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown value of AesKeyLength");

        auto ctx = EVP_CIPHER_CTX_new();
        EVP_CIPHER_CTX_init(ctx);
        EVP_EncryptInit_ex(ctx, cipher, nullptr, nullptr, nullptr);
        int keyLength = EVP_CIPHER_CTX_key_length(ctx);
        int ivLength = EVP_CIPHER_CTX_iv_length(ctx);

        QVector<unsigned char> key(keyLength);
        QVector<unsigned char> iv(ivLength);

        int ok = EVP_BytesToKey(
            cipher, EVP_sha256(),
            m_salt.isEmpty() ? nullptr
            : reinterpret_cast<unsigned char*>(m_salt.data()),
            reinterpret_cast<unsigned char*>(m_password.data()), m_password.length(),
            m_numRounds, key.data(), iv.data());

        EVP_CIPHER_CTX_free(ctx);

        if (ok == 0) return false;

        int res = AES_set_encrypt_key(key.data(), keyLength * 8, m_aesKey);
        if (res != 0) return false;

        initCtr(&m_ctrState, iv.data());

        return true;
    }

    char* CryptFileDevice::encrypt(const char* plainText, qint64 len) {
        auto cipherText = new unsigned char[len];

        qint64 processLen = 0;
        do {
            int maxCipherLen = len > std::numeric_limits<int>::max()
                ? std::numeric_limits<int>::max()
                : len;
            CRYPTO_ctr128_encrypt(
                reinterpret_cast<const unsigned char*>(plainText) + processLen,
                cipherText + processLen, maxCipherLen, m_aesKey, m_ctrState.ivec,
                m_ctrState.ecount, &m_ctrState.num, (block128_f)AES_encrypt);

            processLen += maxCipherLen;
            len -= maxCipherLen;
        } while (len > 0);

        return reinterpret_cast<char*>(cipherText);
    }

    char* CryptFileDevice::decrypt(const char* cipherText, qint64 len) {
        auto plainText = new unsigned char[len];

        qint64 processLen = 0;
        do {
            int maxPlainLen = len > std::numeric_limits<int>::max()
                ? std::numeric_limits<int>::max()
                : len;
            CRYPTO_ctr128_encrypt(
                reinterpret_cast<const unsigned char*>(cipherText) + processLen,
                plainText + processLen, maxPlainLen, m_aesKey, m_ctrState.ivec,
                m_ctrState.ecount, &m_ctrState.num, (block128_f)AES_encrypt);

            processLen += maxPlainLen;
            len -= maxPlainLen;
        } while (len > 0);

        return reinterpret_cast<char*>(plainText);
    }

    bool CryptFileDevice::atEnd() const { return QIODevice::atEnd(); }

    qint64 CryptFileDevice::bytesAvailable() const {
        return QIODevice::bytesAvailable();
    }

    qint64 CryptFileDevice::pos() const { return QIODevice::pos(); }

    bool CryptFileDevice::seek(qint64 pos) {
        bool result = QIODevice::seek(pos);
        if (m_encrypted) {
            m_device->seek(kHeaderLength + pos);
            initCtr(&m_ctrState, m_ctrState.ivec);
        }
        else {
            m_device->seek(pos);
        }

        return result;
    }

    qint64 CryptFileDevice::size() const {
        if (m_device == nullptr) return 0;

        if (!m_encrypted) return m_device->size();

        return m_device->size() - kHeaderLength;
    }

    bool CryptFileDevice::remove() {
        if (m_device == nullptr) return false;

        QString fileName = m_device->fileName();
        if (fileName.isEmpty()) return false;

        if (isOpen()) close();

        bool ok = QFile::remove(fileName);
        if (ok) m_device = nullptr;

        return ok;
    }

    bool CryptFileDevice::exists() const {
        if (m_device == nullptr) return false;

        QString fileName = m_device->fileName();
        if (fileName.isEmpty()) return false;

        return QFile::exists(fileName);
    }

    bool CryptFileDevice::rename(const QString& newName) {
        if (m_device == nullptr) return false;

        QString fileName = m_device->fileName();
        if (fileName.isEmpty()) return false;

        if (isOpen()) close();

        bool ok = QFile::rename(fileName, newName);
        if (ok) setFileName(newName);

        return ok;
    }
}
