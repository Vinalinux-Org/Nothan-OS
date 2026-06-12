#ifndef AM335X_MMC_H
#define AM335X_MMC_H

#define MMC0_BASE           0x48060000

#define MMC_SYSCONFIG       (MMC0_BASE + 0x110)
#define MMC_SYSSTATUS       (MMC0_BASE + 0x114)
#define MMC_CON             (MMC0_BASE + 0x12C)
#define MMC_BLK             (MMC0_BASE + 0x204)
#define MMC_ARG             (MMC0_BASE + 0x208)
#define MMC_CMD             (MMC0_BASE + 0x20C)
#define MMC_RSP10           (MMC0_BASE + 0x210)
#define MMC_DATA            (MMC0_BASE + 0x220)
#define MMC_PSTATE          (MMC0_BASE + 0x224)
#define MMC_HCTL            (MMC0_BASE + 0x228)
#define MMC_SYSCTL          (MMC0_BASE + 0x22C)
#define MMC_STAT            (MMC0_BASE + 0x230)
#define MMC_IE              (MMC0_BASE + 0x234)
#define MMC_ISE             (MMC0_BASE + 0x238)
#define MMC_AC12            (MMC0_BASE + 0x23C)
#define MMC_CAPA            (MMC0_BASE + 0x240)
#define MMC_CUR_CAPA        (MMC0_BASE + 0x248)
#define MMC_ADMAES          (MMC0_BASE + 0x254)

#define MMC_HCTL_DTW_4BIT   (1 << 1)
#define MMC_HCTL_HSPE       (1 << 2)
#define MMC_HCTL_SDBP       (1 << 8)
#define MMC_HCTL_SDVS_1_8V  (0x5 << 9)
#define MMC_HCTL_SDVS_3_0V  (0x6 << 9)
#define MMC_HCTL_SDVS_3_3V  (0x7 << 9)

#define MMC_SYSCTL_ICE      (1 << 0)
#define MMC_SYSCTL_ICS      (1 << 1)
#define MMC_SYSCTL_CEN      (1 << 2)
#define MMC_SYSCTL_CLKD_SHIFT 6
#define MMC_SYSCTL_SRA      (1 << 24)
#define MMC_SYSCTL_SRC      (1 << 25)
#define MMC_SYSCTL_SRD      (1 << 26)

#define MMC_CMD_GO_IDLE         0
#define MMC_CMD_SEND_OP_COND    1
#define MMC_CMD_ALL_SEND_CID    2
#define MMC_CMD_SET_REL_ADDR    3
#define MMC_CMD_SELECT_CARD     7
#define MMC_CMD_SEND_IF_COND    8
#define MMC_CMD_SEND_CSD        9
#define MMC_CMD_READ_SINGLE     17
#define MMC_CMD_READ_MULTIPLE   18

#endif /* AM335X_MMC_H */
