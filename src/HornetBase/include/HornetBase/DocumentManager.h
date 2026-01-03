// DocumentManager.h
#pragma once
#include <memory>
#include <vector>
#include <algorithm>
#include <type_traits>
#include "HornetBaseExport.h"
#include "Document.h"

// Generic document manager that owns a collection of Documents
// (DocumentModel, DocumentChart, etc.), and tracks the active one.
// No Qt dependency here.
class HORNETBASE_EXPORT DocumentManager
{
public:
    using Container = std::vector<std::unique_ptr<Document>>;

    DocumentManager() = default;
    ~DocumentManager() = default;

    DocumentManager(const DocumentManager&) = delete;
    DocumentManager& operator=(const DocumentManager&) = delete;
    DocumentManager(DocumentManager&&) noexcept;
    DocumentManager& operator=(DocumentManager&&) noexcept;

    const Container& documents() const noexcept { return m_documents; }
    Container&       documents()       noexcept { return m_documents; }
    
    Document*        activeDocument() const noexcept { return m_active; }

    // --------------------------------------------------------------------
    // Generic creation for any Document-derived type
    // --------------------------------------------------------------------
    template<typename DocT, typename... Args>
    DocT* createDocument(Args&&... args)
    {
        static_assert(std::is_base_of<Document, DocT>::value,
                      "DocT must derive from Document");

        auto doc = std::make_unique<DocT>(std::forward<Args>(args)...);
        DocT* ptr = doc.get();
        m_documents.emplace_back(std::move(doc));
        ptr->setName("Document " + std::to_string(m_documents.size()));
        setActiveDocument(ptr);
        return ptr;
    }

    // Add a document created elsewhere (e.g. from a factory).
    // Manager takes ownership.
    Document* addDocument(std::unique_ptr<Document> doc);

    // Close and destroy a managed document.
    void closeDocument(Document* doc);

    // Close and destroy all documents.
    void closeAll();

    // Just change which document is considered active (no ownership change).
    void setActiveDocument(Document* doc) noexcept;

    // Convenience: try to cast active document to a specific derived type.
    template<typename DocT>
    DocT* activeAs() const noexcept
    {
        static_assert(std::is_base_of<Document, DocT>::value,
                      "DocT must derive from Document");
        return dynamic_cast<DocT*>(m_active);
    }

private:
    Container m_documents;
    Document* m_active = nullptr;
};
