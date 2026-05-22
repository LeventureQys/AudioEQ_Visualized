#pragma once
#include <QObject>
#include <QVector>

#if defined(AUDIOEQ_LIBRARY)
#  define AUDIOEQ_EXPORT Q_DECL_EXPORT
#else
#  define AUDIOEQ_EXPORT Q_DECL_IMPORT
#endif

class BandHandle;

class AUDIOEQ_EXPORT BandFocusManager : public QObject {
    Q_OBJECT
public:
    explicit BandFocusManager(QObject* parent = nullptr);

    void registerBand(BandHandle* handle);
    void unregisterBand(int index);
    void clear();

    int focusedIndex() const;
    void setFocusedIndex(int index);
    void clearFocus();

signals:
    void focusChanged(int focusedIndex);

private slots:
    void onBandPressed(int index);

private:
    QVector<BandHandle*> m_handles;
    int m_focusedIndex = -1;

    BandHandle* findHandle(int index) const;
};
