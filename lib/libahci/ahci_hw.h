#ifndef __ahci_hw_h
#define __ahci_hw_h

#define bitfld_set(_v, _n, _f) (((_v) & ~_n##_MASK)|(((_f) << _n##_SHIFT) & _n##_MASK))
#define bitfld_get(_v, _n) (((_v) & _n##_MASK) >> _n##_SHIFT)
#define _B(_x) (1LL << (_x))

struct blk_ahci_ata_ident {
#define ATA_IDENT_SERIAL     10
#define ATA_IDENT_FIRMWARE   23
#define ATA_IDENT_MODEL      27
#define ATA_IDENT_SECTNO     60
#define ATA_IDENT_SECTNO48   100
#define ATA_IDENT_SECTSZ     106
#define ATA_IDENT_SECTSZ_MBZ _B(15)
#define ATA_IDENT_SECTSZ_MBO _B(14)
#define ATA_IDENT_SECTSZ_BIG _B(12)
#define ATA_IDENT_SECTSZ_CUSTOM_MASK (_B(15)|_B(14)|_B(12))
#define ATA_IDENT_SECTSZ_CUSTOM      (_B(14)|_B(12))
#define ATA_IDENT_LOGSECTSZ  117
	uint16_t words[256];
} __packed;

struct blk_ahci_hw_hba {

#define CAP_S64A       _B(31)
#define CAP_SNCQ       _B(30)
#define CAP_SSNTF      _B(29)
#define CAP_SMPS       _B(28)
#define CAP_SSS        _B(27)
#define CAP_SALP       _B(26)
#define CAP_SAL        _B(25)
#define CAP_SCLO       _B(24)
#define CAP_ISS_MASK   0x00F00000
#define CAP_ISS_SHIFT  20
#define CAP_SAM        _B(18)
#define CAP_SPM        _B(17)
#define CAP_FBSS       _B(16)
#define CAP_PMD        _B(15)
#define CAP_SSC        _B(14)
#define CAP_PSC        _B(13)
#define CAP_NCS_MASK   0x00001F00
#define CAP_NCS_SHIFT  8
#define CAP_CCCS       _B(7)
#define CAP_EMS        _B(6)
#define CAP_SXS        _B(5)
#define CAP_NP_MASK    0x0000001F
#define CAP_NP_SHIFT   0
	uint32_t cap;

#define GHC_AE         _B(31)
#define GHC_MRSM       _B(2)
#define GHC_IE         _B(1)
#define GHC_HR         _B(0)
	uint32_t ghc;

	uint32_t is;
	uint32_t pi;

#define VS_MAJOR_MASK  0xFFFF0000
#define VS_MAJOR_SHIFT 16
#define VS_MINOR_MASK  0x00000000
#define VS_MINOR_SHIFT 0
	uint32_t vs;

	uint32_t ccc_ctl;
	uint32_t ccc_pts;
	uint32_t em_loc;
	uint32_t em_ctl;
	uint32_t cap2;
	uint32_t bohc;
} __packed;

struct blk_ahci_hw_port {
	uint32_t clb;
	uint32_t clbu;
	uint32_t fb;
	uint32_t fbu;

#define PX_IS_TFES _B(30)
#define PX_IS_HBFS _B(29)
#define PX_IS_HBDS _B(28)
#define PX_IS_IFS  _B(27)


	uint32_t is;
	uint32_t ie;

#define PX_CMD_ICC_MASK  0xF0000000
#define PX_CMD_ICC_SHIFT 28
#define PX_CMD_ASP       _B(27)
#define PX_CMD_ALPE      _B(26)
#define PX_CMD_DLAE      _B(25)
#define PX_CMD_ATAPI     _B(24)
#define PX_CMD_APSTE     _B(23)
#define PX_CMD_FBSCP     _B(22)
#define PX_CMD_ESP       _B(21)
#define PX_CMD_CPD       _B(20)
#define PX_CMD_MPSP      _B(19)
#define PX_CMD_HPCP      _B(18)
#define PX_CMD_PMA       _B(17)
#define PX_CMD_CPS       _B(16)
#define PX_CMD_CR        _B(15)
#define PX_CMD_FR        _B(14)
#define PX_CMD_MPSS      _B(13)
#define PX_CMD_CCS_MASK  0x00001F00
#define PX_CMD_CCS_SHIFT 8
#define PX_CMD_FRE       _B(4)
#define PX_CMD_CLO       _B(3)
#define PX_CMD_POD       _B(2)
#define PX_CMD_SUD       _B(1)
#define PX_CMD_ST        _B(0)
	uint32_t cmd;
	uint32_t rsv0;
	uint32_t tfd;
	uint32_t sig;

#define PX_SSTS_IPM_E  0
#define PX_SSTS_IPM_A  1
#define PX_SSTS_IPM_P  2
#define PX_SSTS_IPM_S  6
#define PX_SSTS_IPM_D  8
#define PX_SSTS_SPD_E  0
#define PX_SSTS_SPD_G1 1
#define PX_SSTS_SPD_G2 2
#define PX_SSTS_SPD_G3 3
#define PX_SSTS_DET_E 0
#define PX_SSTS_DET_D 1
#define PX_SSTS_DET_P 3

#define PX_SSTS_IPM_MASK    0x00000F00
#define PX_SSTS_IPM_SHIFT   8
#define PX_SSTS_SPD_MASK    0x000000F0
#define PX_SSTS_SPD_SHIFT   4
#define PX_SSTS_DET_MASK    0x0000000F
#define PX_SSTS_DET_SHIFT   0
	uint32_t ssts;

	uint32_t sctl;
	uint32_t serr;
	uint32_t sact;
	uint32_t ci;
	uint32_t sntf;
	uint32_t fbs;
	uint32_t rsv1[11];
	uint32_t vendor[4];
} __packed;

struct blk_ahci_hw_cmdhdr {
#define CMDHDR_PRDTL_MASK  0xFFFF0000
#define CMDHDR_PRDTL_SHIFT 16
#define CMDHDR_PMP_MASK    0x0000F000
#define CMDHDR_PMP_SHIFT   12
#define CMDHDR_C           _B(10)
#define CMDHDR_B           _B(9)
#define CMDHDR_R           _B(8)
#define CMDHDR_P           _B(7)
#define CMDHDR_W           _B(6)
#define CMDHDR_A           _B(5)
#define CMDHDR_CFL_MASK    0x0000001F
#define CMDHDR_CFL_SHIFT   0
	uint32_t opts;
	uint32_t prdbc;
	uint32_t ctba;
	uint32_t ctbau;
	uint32_t resv[4];
} __packed;

struct blk_ahci_hw_prdt {
	uint32_t dba;
	uint32_t dbau;
	uint32_t res;

#define PRDT_DBC_I     _B(31)
#define PRDT_DBC_MASK  0x0001FFFF
#define PRDT_DBC_SHIFT 0
	uint32_t dbc;
} __packed;

#define	FIS_TYPE_REG_H2D   0x27
#define	FIS_TYPE_REG_D2H   0x34
#define	FIS_TYPE_DMA_ACT   0x39
#define	FIS_TYPE_DMA_SETUP 0x41
#define	FIS_TYPE_DATA      0x46
#define	FIS_TYPE_BIST      0x58
#define	FIS_TYPE_PIO_SETUP 0x5F
#define	FIS_TYPE_DEV_BITS  0xA1

#define FIS_FLAGS_PM_SHIFT 0
#define FIS_FLAGS_PM_MASK  0x0f
#define FIS_FLAGS_C        0x80
#define FIS_FLAGS_DIR_D2H  0x20
#define FIS_FLAGS_INTR     0x40
#define FIS_FLAGS_CMD      0x80

#define PORT_INT_DHR       0x00000001
#define PORT_INT_PS        0x00000002
#define PORT_INT_DS        0x00000004
#define PORT_INT_SDB       0x00000008
#define PORT_INT_UF        0x00000010
#define PORT_INT_DP        0x00000020
#define PORT_INT_PC        0x00000040
#define PORT_INT_DMP       0x00000080
#define PORT_INT_TFE       0x40000000

#define PRDT_DBC_I _B(31)

struct fis_reg_h2d {
	uint8_t fis;
	uint8_t flags;
	uint8_t cmd;
	uint8_t feature1;

	uint8_t lba0;
	uint8_t lba1;
	uint8_t lba2;
	uint8_t dev;

	uint8_t lba3;
	uint8_t lba4;
	uint8_t lba5;
	uint8_t feature2;

	uint8_t count1;
	uint8_t count2;
	uint8_t icc;
	uint8_t control;

	uint32_t mbz;
} __packed;

struct fis_reg_d2h {
	uint8_t fis;
	uint8_t flags;
	uint8_t status;
	uint8_t error;

	uint8_t lba0;
	uint8_t lba1;
	uint8_t lba2;
	uint8_t dev;

	uint8_t lba3;
	uint8_t lba4;
	uint8_t lba5;
	uint8_t res1;

	uint8_t count1;
	uint8_t count2;
	uint16_t res;

	uint32_t mbz;
} __packed;

#endif
