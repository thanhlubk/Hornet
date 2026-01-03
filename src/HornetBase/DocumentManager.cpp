// DocumentManager.cpp
#include "HornetBase/DocumentManager.h"
#include "HornetBase/Document.h"

Document* DocumentManager::addDocument(std::unique_ptr<Document> doc)
{
    if (!doc)
        return nullptr;

    Document* ptr = doc.get();
    m_documents.emplace_back(std::move(doc));

    if (!m_active)
        m_active = ptr;

    return ptr;
}

void DocumentManager::closeDocument(Document* doc)
{
    if (!doc)
        return;

    auto it = std::find_if(
        m_documents.begin(), m_documents.end(),
        [doc](const std::unique_ptr<Document>& p) { return p.get() == doc; });

    if (it == m_documents.end())
        return;

    if (m_active == doc)
        m_active = nullptr;

    m_documents.erase(it);

    if (!m_active && !m_documents.empty())
        m_active = m_documents.front().get();
}

void DocumentManager::closeAll()
{
    m_documents.clear();
    m_active = nullptr;
}

void DocumentManager::setActiveDocument(Document* doc) noexcept
{
    m_active = doc;
}
