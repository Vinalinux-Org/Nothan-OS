# VinixOS — Commit Style

## 1. Format

```
Type(scope): short description (≤ 70 chars)

[Optional body — chỉ khi cần giải thích WHY]
```

**Không** body cho commit nhỏ (typo, rename, format). Body bắt buộc khi:
- Behavior change mà commit message không nói được hết
- Cần ghi lại HW errata, TRM reference cho future debug
- Touch nhiều file — cần giải thích relationship

---

## 2. Type taxonomy

| Type | Khi nào | Ví dụ |
| --- | --- | --- |
| `Feat` | Tính năng mới | `Feat(mmc): implement omap_hsmmc probe` |
| `Fix` | Bug fix với root cause xác định | `Fix(sched): clear stale state on SIGKILL` |
| `Refactor` | Đổi cấu trúc, không đổi behavior | `Refactor(drivers): drop main.c init calls` |
| `Docs` | Sửa tài liệu / comment | `Docs(driver): add platform_driver template` |
| `Test` | Thêm/sửa test | `Test(mmu): cover unmap edge cases` |

**KHÔNG có**: `WIP`, `Update`, `Change`, `Misc`, `Port`, `Based on`, `Improve`, `Cleanup` (dùng Refactor).

---

## 3. Scope chọn

- 1 module: `Feat(uart): ...`
- 2-3 module liên quan: `Refactor(drivers): ...`
- Build / scripts: `Refactor(scripts): ...`, `Fix(build): ...`
- Toàn project: hiếm — `Refactor(tree): rename folders kebab-case`

---

## 4. Description craft

```
Đúng:
  Refactor(i2c): convert omap-i2c to platform_driver, drop main.c direct calls
  Fix(sched): clear stale task state on SIGKILL
  Feat(mmc): implement omap_hsmmc probe and card init sequence

Sai:
  Update i2c driver                              ← không nói gì
  Fix bug                                         ← không root cause
  WIP: working on mmc                             ← phase marker
  Port omap_serial from Linux                     ← cấm "Port"
  Based on Linux drivers/tty/serial/8250.c        ← cấm "Based on"
  Many fixes and improvements                     ← multi-purpose commit
```

---

## 5. Khi tách commit

1 commit = 1 ý. Nếu phải dùng "and" trong description → cân nhắc tách:

```
Refactor(uart): convert to platform_driver
Refactor(uart): move register defines to header
Fix(uart): handle FIFO overflow on RX
```

vs

```
Refactor(uart): convert to platform_driver and move defines and fix overflow
   ^^^^^^^^^^^^^                                                        ← tách
```

---

## 6. Cấm tuyệt đối

- `Co-Authored-By: Claude` trailer trên commit VinixOS — **không bao giờ**
- `git tag v0.PN-complete` — chỉ commit, không tag phase
- `git commit --amend` commit đã push — luôn tạo commit mới

---

## Phụ lục — Commit Smell

| Smell | Fix |
| --- | --- |
| `Update xxx` | Refactor / Feat / Fix với verb cụ thể |
| `Co-Authored-By: Claude` | Xóa trailer |
| `WIP` | Squash hoặc finalize trước commit |
| `Port from Linux` | Vi phạm — viết lại implementation từ đầu |
| Diff > 500 LOC, message 1 dòng | Tách commit theo ý |


