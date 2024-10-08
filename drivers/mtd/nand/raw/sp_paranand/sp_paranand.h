/* SPDX-License-Identifier: GPL-2.0
 *
 * Parallel NAND Cotroller driver
 *
 * Derived from:
 *	Copyright (C) 2019-2021 Faraday Technology Corp.
 */

extern const struct nand_flash_dev sp_nfc_ids[];

#define ENABLE_DMA_MODE		1
#define NAME_DEFINE_IN_UBOOT		"sp_paranand.0"

/* Frequency Setting (unit: HZ) */
#define CONFIG_SP_CLK_100M		100000000
#define CONFIG_SP_CLK_200M		200000000
#define CONFIG_SP_CLK_400M		400000000

/* Reserved BI byte (bad block information) from spare location, BI_byte = 1~6 */
#define CONFIG_BI_BYTE		0

/* Debug level */
#define CONFIG_SP_DEBUG		1

#define ECC_CONTROL			0x8
#define ECC_ERR_MASK(x)			((x) << 0)
#define ECC_EN(x)			((x) << 8)
#define ECC_BASE			BIT(16)
#define ECC_NO_PARITY			BIT(17)
#define ECC_THRES_BITREG1		0x10
#define ECC_THRES_BITREG2		0x14
#define ECC_CORRECT_BITREG1		0x18
#define ECC_CORRECT_BITREG2		0x1C
#define ECC_INTR_EN			0x20
#define ECC_INTR_THRES_HIT		BIT(1)
#define ECC_INTR_CORRECT_FAIL		BIT(0)
#define ECC_INTR_STATUS			0x24
#define ECC_ERR_THRES_HIT_FOR_SPARE(x)	(1 << (24 + (x)))
#define ECC_ERR_FAIL_FOR_SPARE(x)	(1 << (16 + (x)))
#define ECC_ERR_THRES_HIT(x)		(1 << (8 + (x)))
#define ECC_ERR_FAIL(x)			(1 << (x))

#define ECC_THRES_BIT_FOR_SPARE_REG1	0x34
#define ECC_THRES_BIT_FOR_SPARE_REG2	0x38
#define ECC_CORRECT_BIT_FOR_SPARE_REG1	0x3C
#define ECC_CORRECT_BIT_FOR_SPARE_REG2	0x40
#define DEV_BUSY			0x100
#define GENERAL_SETTING			0x104
#define CE_NUM(x)			((x) << 24)
#define BUSY_RDY_LOC(x)			((x) << 12)
#define CMD_STS_LOC(x)			((x) << 8)
//#define REPORT_ADDR_EN		BIT(4)
#define WRITE_PROTECT			BIT(2)
#define DATA_INVERSE			BIT(1)
#define DATA_SCRAMBLER			BIT(0)
#define MEM_ATTR_SET			0x108
#define BI_BYTE_MASK			(0x7 << 19) //from PNANDC v2.3
#define PG_SZ_512			(0 << 16)
#define PG_SZ_2K			BIT(16)
#define PG_SZ_4K			(2 << 16)
#define PG_SZ_8K			(3 << 16)
#define PG_SZ_16K			(4 << 16)
#define ATTR_COL_CYCLE(x)		(((x) & 0x1) << 12)
#define ATTR_ROW_CYCLE(x)		(((x) & 0x3) << 13)
#define ROW_ADDR_1CYCLE			0
#define ROW_ADDR_2CYCLE			1
#define ROW_ADDR_3CYCLE			2
#define ROW_ADDR_4CYCLE			3
#define COL_ADDR_1CYCLE			0
#define COL_ADDR_2CYCLE			1
#define MEM_ATTR_SET2			0x10C
//#define SPARE_PAGE_MODE(x)		((x) << 0)
//#define SPARE_PROT_EN(x)		((x) << 8)
#define VALID_PAGE(x)			(((x) & 0x3FF) << 16)

#define FL_AC_TIMING0(x)		(0x110 + ((x) << 3))
#define FL_AC_TIMING1(x)		(0x114 + ((x) << 3))
#define FL_AC_TIMING2(x)		(0x190 + ((x) << 3))
#define FL_AC_TIMING3(x)		(0x194 + ((x) << 3))
#define INTR_ENABLE			0x150
//#define INTR_ENABLE_CRC_CHECK_EN(x)	((x) << 8)
#define INTR_ENABLE_STS_CHECK_EN(x)	((x) << 0)
#define INTR_STATUS			0x154
#define STATUS_CMD_COMPLETE(x)		(1 << (16 + (x)))
//#define STATUS_CRC_FAIL(x)		(1 << (8 + (x)))
#define STATUS_FAIL(x)			(1 << (x))
#define READ_STATUS0			0x178
#define NANDC_SW_RESET			0x184
#define NANDC_EXT_CTRL			0x1E0 //from PNANDC v2.4
#define SEED_SEL(x)			(1 << (x))
#define CMDQUEUE_STATUS			0x200
#define CMDQUEUE_STATUS_FULL(x)		(1 << (8 + (x)))
#define CMDQUEUE_STATUS_EMPTY(x)	(1 << (x))
#define CMDQUEUE_FLUSH			0x204

#define CMDQUEUE1(x)			(0x300 + ((x) << 5))
#define CMDQUEUE2(x)			(0x304 + ((x) << 5))
#define CMDQUEUE3(x)			(0x308 + ((x) << 5))
#define CMD_COUNT(x)			((x) << 16)
#define CMDQUEUE4(x)			(0x30C + ((x) << 5))
#define CMD_COMPLETE_EN			BIT(0)
#define CMD_SCALE(x)			(((x) & 0x3) << 2)
#define CMD_DMA_HANDSHAKE_EN		BIT(4)
#define CMD_FLASH_TYPE(x)		(((x) & 0x7) << 5)
#define CMD_INDEX(x)			(((x) & 0x3FF) << 8)
#define CMD_PROM_FLOW			BIT(18)
#define CMD_SPARE_NUM(x)		((((x) - 1) & 0x1F) << 19)
//from PNANDC v2.4, the extended spare_num is 0x304 b[25:24]
#define CMD_EX_SPARE_NUM(x)		(((((x) - 1) >> 5) & 0x3) << 24)
#define SCR_SEED_VAL1(x)		(((x) & 0xff) << 24)
#define SCR_SEED_VAL2(x)		((((x) & 0x3fff) >> 8) << 26)

#define CMD_BYTE_MODE			BIT(28)
#define CMD_START_CE(x)			(((x) & 0x7) << 29)

#define BMC_REGION_STATUS		0x400
#define BMC_REGION_BUF_EMPTY(x)		(1 << ((x) + 24))
#define BMC_REGION_HALT(x)		(1 << ((x) + 16))
#define BMC_REGION_FULL(x)		(1 << ((x) + 8))
#define BMC_REGION_EMPTY(x)		(1 << (x))
#define BMC_REGION_SW_RESET		0x428  //WO

#define REVISION_NUM			0x500
#define FEATURE_1			0x504
#define MAX_SPARE_DATA_128BYTE		BIT(15) //from PNANDC v2.4
#define AHB_SLAVE_MODE_ASYNC(x)		(1 << ((x) + 27))  //Asynchronous mode
#define DDR_IF_EN			BIT(31)  //Enable DDR interface
#define AHB_SLAVEPORT_SIZE		0x508
#define AHB_SLAVE_SPACE_512B		BIT(0)
#define AHB_SLAVE_SPACE_1KB		BIT(1)
#define AHB_SLAVE_SPACE_2KB		BIT(2)
#define AHB_SLAVE_SPACE_4KB		BIT(3)
#define AHB_SLAVE_SPACE_8KB		BIT(4)
#define AHB_SLAVE_SPACE_16KB		BIT(5)
#define AHB_SLAVE_SPACE_32KB		BIT(6)
#define AHB_SLAVE_SPACE_64KB		BIT(7)
#define AHB_RETRY_EN(ch_index)		(1 << ((ch_index) + 8))
#define AHB_PREFETCH(ch_index)		(1 << ((ch_index) + 12))
#define AHB_PRERETCH_LEN(x_words)	((x_words) << 16)
#define AHB_SPLIT_EN			BIT(25)
#define GLOBAL_RESET			0x50C
#define AHB_SLAVE_RESET			0x510
#define DQS_DELAY			0x520
#define PROGRAMMABLE_OPCODE		0x700
#define PROGRAMMABLE_FLOW_CONTROL	0x2000
#define SPARE_SRAM			0x1000

#define FIXFLOW_READID			0x5F
#define FIXFLOW_RESET			0x65
#define FIXFLOW_READSTATUS		0x96
/* FIX_FLOW_INDEX for small page */
#define SMALL_FIXFLOW_READOOB		0x249
#define SMALL_FIXFLOW_PAGEREAD		0x23E
#define SMALL_FIXFLOW_PAGEWRITE		0x26C
#define SMALL_FIXFLOW_WRITEOOB		0x278
#define SMALL_FIXFLOW_ERASE		0x2C1

/* FIX_FLOW_INDEX for large page */
#define LARGE_FIXFLOW_BYTEREAD		0x8A
#define LARGE_FIXFLOW_READOOB		0x3E
#define LARGE_FIXFLOW_PAGEREAD		0x1C
#define LARGE_FIXFLOW_PAGEREAD_W_SPARE	0x48
#define LARGE_PAGEWRITE_W_SPARE		0x26
#define LARGE_PAGEWRITE			0x54
#define LARGE_FIXFLOW_WRITEOOB		0x33
#define LARGE_FIXFLOW_ERASE		0x68

/* FIX_FLOW_INDEX for ONFI change mode */
#define ONFI_FIXFLOW_GETFEATURE		0x22B
#define ONFI_FIXFLOW_SETFEATURE		0x232
#define ONFI_FIXFLOW_SYNCRESET		0x21A

#define max_2(a, b)			max_t(typeof(a), a, b)
#define min_2(a, b)			min_t(typeof(a), a, b)

#define max_3(a, b, c) ({ \
	typeof(a) _max_a = (a); \
	typeof(b) _max_b = (b); \
	typeof(c) _max_c = (c); \
	typeof(a) _max_ab = max(_max_a, _max_b); \
	max(_max_ab, _max_c); \
})

#define min_3(a, b, c) ({ \
	typeof(a) _min_a = (a); \
	typeof(b) _min_b = (b); \
	typeof(c) _min_c = (c); \
	typeof(a) _min_ab = min(_min_a, _min_b); \
	min(_min_ab, _min_c); \
})

#define max_4(a, b, c, d) ({ \
	typeof(a) _max_a = (a); \
	typeof(b) _max_b = (b); \
	typeof(c) _max_c = (c); \
	typeof(d) _max_d = (d); \
	typeof(a) _max_abc = max_3(_max_a, _max_b, _max_c); \
	max(_max_abc, _max_d); \
})

#define min_4(a, b, c, d) ({ \
	typeof(a) _min_a = (a); \
	typeof(b) _min_b = (b); \
	typeof(c) _min_c = (c); \
	typeof(d) _min_d = (d); \
	typeof(a) _min_abc = min_3(_min_a, _min_b, _min_c); \
	min(_min_abc, _min_d); \
})

#define MAX_CE				1
#define MAX_CHANNEL			1

/* For recording the command execution status*/
#define CMD_SUCCESS			0
#define CMD_CRC_FAIL			BIT(1)
#define CMD_STATUS_FAIL			BIT(2)
#define CMD_ECC_FAIL_ON_DATA		BIT(3)
#define CMD_ECC_FAIL_ON_SPARE		BIT(4)

/* Feature Setting */
#define CONFIG_PAGE_MODE
//#define CONFIG_SECTOR_MODE

#if CONFIG_SP_DEBUG > 0
	#define TAG "Parallel Nand: "
	#define sp_nfc_dbg(fmt, ...) pr_info(TAG fmt, ##__VA_ARGS__)
#else
	#define sp_nfc_dbg(fmt, ...) do {} while (0)
#endif

#if CONFIG_SP_DEBUG > 0
	#define DBGLEVEL1(x)	x
#else
	#define DBGLEVEL1(x)
#endif

#if CONFIG_SP_DEBUG > 1
	#define DBGLEVEL2(x)	x
#else
	#define DBGLEVEL2(x)
#endif

enum flashtype {
	LEGACY_FLASH = 0,
	ONFI2,
	ONFI3,
	TOGGLE1,
	TOGGLE2,
};

struct sp_pnandchip_attr {
	char *name;
	int sparesize;
	int ecc;
	int eccbaseshift;
	int ecc_spare;
	int block_boundary;
	int crc;
	enum flashtype flash_type;
};

struct sp_nfc_chip_timing {
	u8		twh;
	u8		tch;
	u8		tclh;
	u8		talh;
	u8		tcalh;
	u8		twp;
	u8		treh;
	u8		tcr;
	u8		trsto;
	u8		treaid;
	u8		trea;
	u8		trp;
	u8		twb;
	u8		trb;
	u8		twhr;
	u32		twhr2;
	u8		trhw;
	u8		trr;
	u8		tar;
	u8		trc;
	u32		tadl;
	u8		trhz;
	u32		tccs;
	u8		tcs;
	u8		tcs2;
	u8		tcls;
	u8		tclr;
	u8		tals;
	u8		tcals;
	u8		tcal2;
	u8		tcres;
	u8		tcdqss;
	u8		tdbs;
	u32		tcwaw;
	u8		twpre;
	u8		trpre;
	u8		twpst;
	u8		trpst;
	u8		twpsth;
	u8		trpsth;
	u8		tdqshz;
	u8		tdqsck;
	u8		tcad;
	u8		tdsl;
	u8		tdsh;
	u8		tdqsl;
	u8		tdqsh;
	u8		tdqsd;
	u8		tckwr;
	u8		twrck;
	u8		tck;
	u8		tcals2;
	u8		tdqsre;
	u8		twpre2;
	u8		trpre2;
	u8		tceh;
};

struct cmd_feature {
	u32 cq1;
	u32 cq2;
	u32 cq3;
	u32 cq4;
	u8 row_cycle;
	u8 col_cycle;
};

struct sp_nfc {
	struct nand_chip chip;
	struct nand_controller controller;
	void __iomem *regs;
	struct clk *clk;
	struct reset_control *rstc;
	int sel_chip;
	int cur_cmd;
	int page_addr;
	int column;
	int byte_ofs;
	struct device *dev;
	int cur_chan;
	int valid_chip[MAX_CHANNEL];
	int scan_state;
	int read_state;	//struct nand_chip chip->state is removed
	int cmd_status;
	char flash_raw_id[5];
	int flash_type;
	int large_page;
	int eccbasft;
	int max_spare; //max spare data bytes (SP v2.4: 32/128 bytes)
	int spare_ch_offset; //register 0x1000 channel offset (SP v2.4: 0x80 = shift 7)
	int spare;
	int protect_spare;
	int useecc;
	int useecc_spare;
	int block_boundary;	//addr space of block (unit: page)
	int seed_val; //Scramble Seed bit [7:0] and [13:8] in cq1 and cq2
	int sector_per_page;
	int inverse;
	int scramble;
	int clkfreq;
	int timing_mode;
	int ddr_enable; // Set DDR mode if nand support sync interface
	const char *name;
	struct dma_chan *dmac;

	unsigned long priv ____cacheline_aligned;
};

void sp_nfc_select_chip(struct nand_chip *chip, int cs);
void sp_nfc_abort(struct nand_chip *chip);
void sp_nfc_regdump(struct nand_chip *chip);

int sp_nfc_issue_cmd(struct nand_chip *chip, struct cmd_feature *cmd_f);
void sp_nfc_set_default_timing(struct nand_chip *chip);
int sp_nfc_wait(struct nand_chip *chip);
void sp_nfc_fill_prog_code(struct nand_chip *chip, int location, int cmd_index);
void sp_nfc_fill_prog_flow(struct nand_chip *chip, int *program_flow_buf, int buf_len);
int sp_nfc_read_page(struct nand_chip *chip, u8 *buf, int oob_required, int page);
int sp_nfc_write_page(struct nand_chip *chip, const u8 *buf, int oob_required, int page);

int sp_nfc_read_page_by_dma(struct nand_chip *chip, u8 *buf, int oob_required, int page);
int sp_nfc_write_page_by_dma(struct nand_chip *chip, const u8 *buf, int oob_required, int page);

int sp_nfc_read_oob(struct nand_chip *chip, int page);
int sp_nfc_write_oob(struct nand_chip *chip, int page);
