# EGOS 内核内存管理系统 - 虚拟内存空间管理

---

## 1. VirtualMemorySpace（虚拟内存空间）概述

### 1.1 基本概念

VirtualMemorySpace (VMS) 是 EGOS 中为每个进程/执行上下文提供的独立地址空间抽象。它封装了：

- **虚拟内存区域集合**：通过红黑树组织的所有 VMR
- **专属页表**：该地址空间的 ExtendedPageTableRoot
- **地址范围**：可用的虚拟地址区间

### 1.2 VMS 的核心组成

| 组成部分 | 作用 |
|----------|------|
| RegionTree（红黑树） | 存储所有虚拟内存区域，按虚拟地址排序 |
| PageTableRoot | 该地址空间的页表根节点，用于虚拟 - 物理地址映射 |
| Range | 定义该 VMS 可用的虚拟地址范围 {起始地址，长度} |

### 1.3 VMS 与进程的关系

- 每个进程拥有独立的 VMS，实现地址空间隔离
- 不同进程的 VMS 可以映射到相同的物理页框（共享内存、共享库等）
- 内核也有自己的 VMS，用于管理内核对象和用户栈

---

## 2. VirtualMemoryRegion（虚拟内存区域）

### 2.1 基本概念

VirtualMemoryRegion (VMR) 是 VMS 中的一个连续虚拟地址区间，具有统一的属性：

- **相同的访问权限**（读/写/执行）
- **相同的内存类型**（匿名、文件映射、内核区域等）
- **相同的内存操作策略**（COW、SHARE、COPY 等）

### 2.2 VMR 的组织方式

所有 VMR 通过红黑树按虚拟地址排序存储

### 2.3 VMR 的生命周期

```
创建 → 插入红黑树 → 使用 → 可能分裂/合并 → 最终擦除
```

---

## 3. 虚拟内存区域类型

### 3.1 区域分类

| 类型 | 描述 | 典型用途 |
|------|------|----------|
| **HOLE**（空洞） | 未分配的地址空间 | VMS 初始化时的默认状态，等待分配 |
| **KERNEL**（内核） | 内核使用的内存区域 | 内核对象、线程栈等 |
| **ANON**（匿名） | 无 backing store 的内存 | 堆、栈、malloc 分配的内存 |
| **FILE**（文件） | 映射自文件的内存 | shared objects、mmap 文件 |

### 3.2 区域类型转换流程

```
HOLE ──分配──> ANON/FILE/KERNEL
                    │
                    ▼
                使用期间...
                    │
                    ▼
              擦除后返回 HOLE
```

---

## 4. 内存区域属性与标志

### 4.1 访问权限标志

| 标志 | 含义 |
|------|------|
| WRITABLE | 可写（默认只读） |
| USER | 用户态可访问（否则仅内核态） |
| NOT_EXECUTABLE | 不可执行（NX 位，增强安全性） |

### 4.2 特殊属性标志

| 标志 | 含义 |
|------|------|
| SHARED | 共享区域，多个 VMS 可引用同一物理页 |
| LAZY_LOAD | 延迟加载，访问时才真正分配物理页 |

---

## 5. VirtualMemoryRegionInfo（区域信息结构）

### 5.1 核心字段概览

| 字段类别 | 内容 |
|----------|------|
| 地址范围 | Range {begin, length} - 虚拟地址区间 |
| 标志位 | Flags16 - 权限和属性组合 |
| 操作 ID | Uint8 - 引用 MemoryOperations 的索引 |
| 文件指针 | File* - FILE 类型区域对应的文件（可选） |
| 文件偏移 | Index64 - 文件映射的起始偏移量 |

### 5.2 Info 与 VMR 的关系

```
VirtualMemoryRegion (红黑树节点)
    ├── treeNode: RBtreeNode (用于红黑树组织)
    ├── info: VirtualMemoryRegionInfo (区域属性)
    └── sharedFrames: VirtualMemoryRegionSharedFrames* (共享页框管理，可选)
```

---

## 6. 共享内存机制

### 6.1 VirtualMemoryRegionSharedFrames

当多个 VMR 共享相同的物理页框时（如共享库、共享内存），使用 SharedFrames 结构跟踪：

- **虚拟基地址**：该共享区域的起始虚拟地址
- **引用计数**：有多少个 VMR 引用这些页框
- **页框向量**：存储所有共享页框的索引列表
