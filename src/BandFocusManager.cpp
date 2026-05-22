#include "BandFocusManager.h"
#include "BandHandle.h"

BandFocusManager::BandFocusManager(QObject* parent)
    : QObject(parent)
{
}

void BandFocusManager::registerBand(BandHandle* handle)
{
    if (!handle)
        return;

    int idx = handle->bandIndex();
    if (idx < 0)
        return;

    while (m_handles.size() <= idx)
        m_handles.append(nullptr);

    BandHandle* existing = m_handles.at(idx);
    if (existing)
        disconnect(existing, &BandHandle::bandPressed, this, &BandFocusManager::onBandPressed);

    m_handles[idx] = handle;
    connect(handle, &BandHandle::bandPressed, this, &BandFocusManager::onBandPressed);
}

void BandFocusManager::unregisterBand(int index)
{
    BandHandle* handle = findHandle(index);
    if (!handle)
        return;

    disconnect(handle, &BandHandle::bandPressed, this, &BandFocusManager::onBandPressed);
    if (m_focusedIndex == index)
        clearFocus();

    m_handles[index] = nullptr;
}

void BandFocusManager::clear()
{
    for (int i = 0; i < m_handles.size(); ++i) {
        BandHandle* h = m_handles.at(i);
        if (h) {
            disconnect(h, &BandHandle::bandPressed, this, &BandFocusManager::onBandPressed);
            m_handles[i] = nullptr;
        }
    }
    m_focusedIndex = -1;
}

int BandFocusManager::focusedIndex() const
{
    return m_focusedIndex;
}

void BandFocusManager::setFocusedIndex(int index)
{
    if (index == m_focusedIndex)
        return;

    BandHandle* oldHandle = findHandle(m_focusedIndex);
    if (oldHandle)
        oldHandle->setFocused(false);

    BandHandle* newHandle = findHandle(index);
    if (newHandle)
        newHandle->setFocused(true);

    m_focusedIndex = (newHandle && index >= 0) ? index : -1;
    emit focusChanged(m_focusedIndex);
}

void BandFocusManager::clearFocus()
{
    setFocusedIndex(-1);
}

void BandFocusManager::onBandPressed(int index)
{
    setFocusedIndex(index);
}

BandHandle* BandFocusManager::findHandle(int index) const
{
    if (index >= 0 && index < m_handles.size())
        return m_handles.at(index);
    return nullptr;
}
