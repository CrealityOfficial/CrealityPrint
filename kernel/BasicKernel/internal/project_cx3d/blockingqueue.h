#ifndef BLOCKINGQUEUE_H
#define BLOCKINGQUEUE_H
#include <QCoreApplication>
#include <QWaitCondition>
#include <QQueue>
#include <QMutex>
#include <QThread>
#include <QDebug>

template <typename T>
class BlockingQueue
{
public:
    BlockingQueue() {}
    void put(const T& value)
    {
        QMutexLocker locker(&m_mutex);
        m_queue.enqueue(value);
        m_condition.wakeOne();   //唤醒等待队列中的一个线程(来自wait)
    }
    T take()
    {
        QMutexLocker locker(&m_mutex);
        while (m_queue.isEmpty()) {
            m_condition.wait(&m_mutex);
        }
        return m_queue.dequeue();
    }
    void cleanQueue() {
        QMutexLocker locker(&m_mutex);
        m_queue.clear();
        m_condition.wakeOne();
    }
    bool isEmpty() const
    {
        QMutexLocker locker(&m_mutex);
        return m_queue.isEmpty();
    }
    int size() const
    {
        QMutexLocker locker(&m_mutex);
        return m_queue.size();
    }

private:
    QQueue<T> m_queue;
    mutable QMutex m_mutex;
    QWaitCondition m_condition;
};
#endif // CX3DAUTOSAVEPROJECT_H