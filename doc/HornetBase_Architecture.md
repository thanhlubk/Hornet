# HornetBase Project Architecture

HornetBase is the core data management layer of the Hornet project. It provides a robust, transaction-aware object-relational mapping (ORM) like system designed for high-performance engineering data (Nodes, Elements, Results, etc.).

## 1. Project Structure

The project is organized around a central **DatabaseSession** that manages multiple data stores called **Pools**.

- **DatabaseSession**: The main interface for users. It controls transactions, undo/redo logic, and routes operations to the appropriate Pools.
- **Pools**: Specialized containers for data items.
  - **PoolUnique**: Stores objects of a single type (e.g., `HINode`). Each type has its own chunk-based memory pool.
  - **PoolMix**: Stores objects of multiple types (e.g., different types of `HIElement`) in shared memory chunks to reduce fragmentation and improve cache locality for heterogeneous data.
- **HItem**: The base class for all data objects stored in the database.
- **HCursor**: A handle or "envelope" that manages metadata (flags, status) for an `HItem`.
- **TransactionManager**: Manages the lifecycle of transactions, including recording operations and managing undo/redo stacks.

---

## 2. HItem and HCursor: The Object Model

In HornetBase, data is split into two parts: the **payload** (`HItem`) and the **handle** (`HCursor`).

### HItem (The Data)
- The base class for all domain objects (Nodes, Elements, etc.).
- Inherits from `HItem` and implements virtual methods for type identification.
- Stores the actual engineering data (coordinates, connectivity, values).
- Stores a pointer to its associated `HCursor`.

### HCursor (The Metadata)
- A lightweight object that acts as a stable handle for an `HItem`.
- Stores metadata that is independent of the engineering payload:
  - **ID**: The unique identifier.
  - **Flags**: Bitmasks for selection, visibility, etc.
  - **Status/RenderState**: Internal state for the engine.
- Separation allows the engine to manipulate selection or visibility without triggering a full "Modify" transaction on the item's engineering data.

---

## 3. DatabaseSession: How it Works

`DatabaseSession` acts as a coordinator. It does not store data directly but owns a collection of `Pool` objects.

### Lifecycle of an Operation
1.  **Check Transaction**: All mutating operations (`emplace`, `erase`, `checkOut`) require an open transaction.
2.  **Routing**: Based on the item type (e.g., `HINode` or `HIElementHex8`), the session identifies the correct `CategoryType` and retrieves the corresponding `Pool`.
3.  **Operation Execution**: The session calls the pool's method to perform the work and registers the operation with the `TransactionManager`.

---

## 4. Working with HItems

### Adding a New HItem Type
To create a new data type in HornetBase:
1.  Inherit from `HItem`.
2.  Use the `DECLARE_ITEM_TAGS` macro in the header. This automatically registers the type with the `HItemManager`.
    ```cpp
    class MyNewItem : public HItem {
        DECLARE_ITEM_TAGS(MyNewItem, CategoryType::CatNode, ItemType::ItemMyType)
    public:
        MyNewItem(Id id, HCursor* cursor, HItemCreatorToken tok) : HItem(id, cursor, tok) {}
        // ... your data ...
    };
    ```

### Retrieving and Modifying Data
All data access should go through the `DatabaseSession`.

- **Retrieve (Read-only)**: Use `get<T>(id)`. Returns a `const T*`.
- **Modify (Read-write)**: Use `checkOut<T>(id)`. This notifies the transaction system that the object is about to change and returns a mutable `T*`.
- **Creation**: Use `emplace<T>(id, args...)`. Constructs the object directly in the pool.

---

## 5. Data Storage and Management

### Memory Pools
Data is stored in **Chunks** (typically 1 MiB blocks). This avoids frequent small allocations and ensures data is contiguous in memory.

#### PoolUnique
- **Mapping**: `ItemType -> ChunkItem<T>`
- **Usage**: Best for types with millions of identical instances (like Nodes).
- **ID Management**: `Id` must be unique within the specific `ItemType`.

#### PoolMix
- **Mapping**: `(ItemType, Id) -> MixLoc (ChunkIndex, Offset)`
- **Usage**: Best for heterogeneous types that are often processed together (like various Element types).
- **ID Management**: Uses a `Key{ItemType, Id}`, allowing the same numeric ID to be used for different types within the same pool.
- **Management**:
  - **emplace**: Uses placement `new` to construct objects in aligned offsets within chunks. If a chunk is full, it attempts to `compact_all()` to reclaim holes or allocates a new chunk.
  - **erase**: Destroys the object in-place and leaves a "hole". Physical memory is not reclaimed immediately to avoid shifting other objects, which would invalidate pointers. Reclaiming happens during a global `compact_all()` (triggered when memory is tight).

---

## 6. Transaction Mechanism

### How Transactions Work
A transaction moves through several states:

1.  **Begin**: `beginTransaction()` marks the session as "Open".
2.  **Interaction**:
    -   **emplace<T>(id)**: The object is created in the pool immediately. The `TransactionManager` notes this as an `Emplace` operation.
    -   **checkOut<T>(id)**: Used for modifications. If it's the first time the object is touched in this transaction, a **BEFORE snapshot** is captured (serialized into a string). The operation is noted as `Modify`.
    -   **erase<T>(id)**: A **BEFORE snapshot** is captured. The object is *not* immediately deleted from the pool (deferred erase) to allow other code in the same transaction to still access it.
3.  **Commit**: `commitTransaction()` finishes the work.
    -   An **AFTER snapshot** is captured for all `Modify` and `Emplace` operations.
    -   `Erase` operations are finalized (physical deletion from the pool).
    -   The transaction (with all snapshots) is pushed onto the **Undo Stack**.
4.  **Rollback**: `rollbackTransaction()` cancels the work.
    -   `Modify` operations are reverted using their `BEFORE` snapshots.
    -   `Emplace` operations result in immediate physical deletion of the new objects.
    -   The transaction is discarded.

### Snapshots and Macros
A **Snapshot** is a binary-serialized representation of an `HItem`'s state, stored as an `std::string`.

#### Auto-Snapshotting with Macros
To allow a class to automatically capture its member variables, use the `DEFINE_TRANSACTION_EXCHANGE` macro:

```cpp
class MyItem : public HItem {
    double m_value;
    std::vector<int> m_data;

    // Implements captureTransactionItem and restoreTransactionItem
    DEFINE_TRANSACTION_EXCHANGE(&MyItem::m_value, &MyItem::m_data)
};
```

-   **Generation**: Uses `TransactionHelper` and `captureTransactionItem()` (macros like `DEFINE_TRANSACTION_EXCHANGE` simplify this) to serialize/deserialize members.
-   **Storage**: Held in `m_mapCurrentTransaction` during the transaction, then moved to the `m_vecUndoStack` upon commit.
    -   During a transaction: Held in a map inside the `TransactionManager`.
    -   After commit: Stored in the `m_vecUndoStack`.
    -   After undo: Stored in the `m_vecRedoStack`.

### Undo and Redo Logic
-   **Undo**: 
    -   For `Modify`: Applies the `BEFORE` snapshot.
    -   For `Emplace`: Deletes the object.
    -   For `Erase`: Re-creates the object and applies the `BEFORE` snapshot.
-   **Redo**:
    -   For `Modify`: Applies the `AFTER` snapshot.
    -   For `Emplace`: Re-creates the object and applies the `AFTER` snapshot.
    -   For `Erase`: Deletes the object.

---

## 7. Notification Procedure (NotifyDispatcher)

The **NotifyDispatcher** provides a decoupled communication mechanism that allows HornetBase to interact with other projects (e.g., HornetView, HornetMain) without direct dependencies.

### Key Concepts
- **Observer Pattern**: Components "attach" to the dispatcher to listen for specific `MessageType` events.
- **Thread Safety**: The dispatcher is thread-safe, using internal mutexes to manage observer slots.
- **Lifetime Management**: Observers hold a `weak_ptr` to the dispatcher's core. This ensures that if the dispatcher (which is typically owned by the `DatabaseSession` or `Document`) is destroyed, the observers will simply stop receiving notifications instead of causing a crash.

### Message Structure
- **MessageType**: An enum defining events such as `DataModified`, `TxCommit`, `ViewRequestRedraw`, etc.
- **MessageParam**: A portable, pointer-sized word (`std::uintptr_t`) used to pass data (IDs, pointers, or flags) along with the message.

### Communication Flow
1.  **Subscription**: An external project (e.g., the 3D View) attaches a callback to the `DatabaseSession`'s dispatcher.
    ```cpp
    // Example: Subscribing to any data modification
    m_observer = session.getNotifyDispatcher().attach(this, &MyView::onDataChanged);
    ```
2.  **Notification**: When a transaction is committed or an undo/redo is applied, the `DatabaseSession` calls `notify`.
3.  **Broadcast**: The dispatcher iterates through all active slots and executes the registered callbacks.
4.  **Decoupling**: This allows the database to notify the UI to refresh or the selection manager to update without the database knowing anything about the UI or selection logic.

