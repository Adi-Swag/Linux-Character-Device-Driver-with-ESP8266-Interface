# Linux Kernel Character Driver — Function Reference

> Driver: `complete_driver` | Two minor devices: `1st_device`, `2nd_device`

---

## Analogy: The Driver as a Post Office

Think of the kernel driver as running **two post office counters** (the two devices) inside a government building (the kernel).

| Driver Concept | Analogy |
|---|---|
| `alloc_chrdev_region` | Government assigns you two counter numbers (major + minor) |
| `cdev_init` + `cdev_add` | You install the counters and open them for business |
| `class_create` + `device_create` | You put up signboards so people (userspace) can find the counters |
| `my_open` | A customer walks up to a specific counter |
| `my_write` | Customer drops off a letter (data) at the counter |
| `my_read` | Customer picks up a letter from the counter |
| `my_close` | Customer leaves the counter |
| `device_destroy` + `class_destroy` + `cdev_del` + `unregister_chrdev_region` | You close the counters, remove signboards, and return the counter numbers |

---

## Init & Exit Functions

### `alloc_chrdev_region`

```c
alloc_chrdev_region(&dev, 0, 2, "complete_driver")
```

Asks the kernel to dynamically assign a **major number** and a range of **minor numbers**.

| Argument | Meaning |
|---|---|
| `&dev` | Output: kernel fills this `dev_t` with the allocated major + first minor |
| `0` | Requested first minor number (0 = let kernel decide) |
| `2` | How many consecutive minor numbers to allocate (one per device) |
| `"complete_driver"` | Name shown in `/proc/devices` |

---

### `cdev_init`

```c
cdev_init(&my_cdev, &fops)
```

Initialises a `cdev` struct and links it to your file operations table.

| Argument | Meaning |
|---|---|
| `&my_cdev` | The `cdev` struct to initialise |
| `&fops` | Pointer to your `file_operations` — tells kernel which functions handle open/read/write/close |

---

### `cdev_add`

```c
cdev_add(&my_cdev, dev, 2)
```

Registers the `cdev` with the kernel — makes the device **live**.

| Argument | Meaning |
|---|---|
| `&my_cdev` | The `cdev` to register |
| `dev` | Starting device number (major + first minor) |
| `2` | Number of consecutive minors this `cdev` handles |

---

### `class_create`

```c
class_create("functioning_drivers")
```

Creates a device **class** — a logical grouping that appears under `/sys/class/`.  
This is what allows `udev` to automatically create `/dev/` entries.

| Argument | Meaning |
|---|---|
| `"functioning_drivers"` | Name of the class, visible at `/sys/class/functioning_drivers/` |

---

### `device_create`

```c
device_create(dev_class, NULL, dev,     NULL, "1st_device")
device_create(dev_class, NULL, dev + 1, NULL, "2nd_device")
```

Creates an actual device entry under the class, which `udev` uses to create `/dev/1st_device` and `/dev/2nd_device`.

| Argument | Meaning |
|---|---|
| `dev_class` | The class this device belongs to |
| `NULL` | Parent device (NULL = no parent) |
| `dev` / `dev + 1` | The device number (major + minor) for this specific device |
| `NULL` | Driver data (not used here) |
| `"1st_device"` / `"2nd_device"` | Name that appears in `/dev/` |

---

### `unregister_chrdev_region`

```c
unregister_chrdev_region(dev, 2)
```

Returns the allocated major/minor numbers back to the kernel.

| Argument | Meaning |
|---|---|
| `dev` | Starting device number to free |
| `2` | How many consecutive minors to free |

---

### `cdev_del`

```c
cdev_del(&my_cdev)
```

Unregisters and removes the `cdev` from the kernel. No argument complexity — just pass the `cdev` pointer.

---

### `class_destroy` / `device_destroy`

```c
device_destroy(dev_class, dev)
device_destroy(dev_class, dev + 1)
class_destroy(dev_class)
```

`device_destroy` removes a specific `/dev/` entry. `class_destroy` removes the class from `/sys/class/`.

| Argument | Meaning |
|---|---|
| `dev_class` | The class the device belongs to |
| `dev` / `dev+1` | Which device number to destroy |

---

## File Operation Functions

These are called by the kernel whenever a userspace process interacts with `/dev/1st_device` or `/dev/2nd_device`.

---

### `my_open`

```c
static int my_open(struct inode *inode, struct file *file)
```

Called when a process does `open("/dev/1st_device", ...)`.

| Argument | Meaning |
|---|---|
| `inode` | Kernel's internal representation of the file — contains the device number |
| `file` | The open file instance for this specific `open()` call |

**Key call inside:**
```c
iminor(inode)  // extracts the minor number → tells you which device was opened
```

---

### `my_close`

```c
static int my_close(struct inode *inode, struct file *file)
```

Called when a process closes the file descriptor. Same arguments as `my_open`.  
Mapped to `.release` in `fops` — the kernel calls this when the **last** reference to the file is closed.

---

### `my_read`

```c
static ssize_t my_read(struct file *filp, char __user *buff, size_t len, loff_t *off)
```

Called when userspace does `read(fd, buf, n)`.

| Argument | Meaning |
|---|---|
| `filp` | The open file — used here to get the minor number via `iminor(file_inode(filp))` |
| `buff` | **Userspace** buffer to copy data into (cannot dereference directly — must use `copy_to_user`) |
| `len` | How many bytes the user wants to read |
| `*off` | Current file offset — where in the buffer to start reading from |

**Key call inside:**
```c
copy_to_user(buff, kernel_buffer[minor] + *off, to_copy)
// Safely copies from kernel memory → userspace memory
// Returns 0 on success, non-zero if copy failed (→ return -EFAULT)
```

---

### `my_write`

```c
static ssize_t my_write(struct file *filp, const char __user *buff, size_t len, loff_t *off)
```

Called when userspace does `write(fd, buf, n)`.

| Argument | Meaning |
|---|---|
| `filp` | The open file — used to identify which device |
| `buff` | **Userspace** buffer holding data to write (read-only, must use `copy_from_user`) |
| `len` | How many bytes the user wants to write |
| `*off` | Current file offset (intentionally ignored here — every write overwrites from index 0) |

**Key call inside:**
```c
copy_from_user(kernel_buffer[minor], buff, len)
// Safely copies from userspace memory → kernel memory
// Returns 0 on success, non-zero on failure (→ return -EFAULT)
```

---

## Helper Macros / Functions Used

| Function / Macro | What it does |
|---|---|
| `MAJOR(dev)` | Extracts the major number from a `dev_t` |
| `MINOR(dev)` | Extracts the minor number from a `dev_t` |
| `iminor(inode)` | Gets minor number from an `inode` — used in `open`/`close` |
| `file_inode(filp)` | Gets the `inode` from a `file` pointer — used in `read`/`write` |
| `IS_ERR(ptr)` | Checks if a pointer is actually an error code (kernel convention for returning errors as pointers) |
| `min(a, b)` | Kernel-safe min — used to not read more than what's in the buffer |
| `pr_info(...)` | Kernel equivalent of `printf` — output goes to `dmesg` |
| `pr_err(...)` | Same as `pr_info` but marks message as error level |

---

## Full Flow Summary

```
insmod complete_driver.ko
    │
    ├─ alloc_chrdev_region    → get major:minor0, major:minor1
    ├─ cdev_init + cdev_add   → register fops with kernel
    ├─ class_create           → /sys/class/functioning_drivers/
    └─ device_create ×2       → /dev/1st_device, /dev/2nd_device

open("/dev/1st_device")  →  my_open   (minor=0)
write(fd, "hello", 5)    →  my_write  (copies "hello" to kernel_buffer[0])
read(fd, buf, 5)         →  my_read   (copies kernel_buffer[0] back to user)
close(fd)                →  my_close

rmmod complete_driver
    │
    ├─ device_destroy ×2
    ├─ class_destroy
    ├─ cdev_del
    └─ unregister_chrdev_region
```

---

*Module: `complete_driver` | Author: Sai Adithya | Version: 1.1.1*