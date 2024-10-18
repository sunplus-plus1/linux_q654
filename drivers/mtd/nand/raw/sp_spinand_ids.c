// SPDX-License-Identifier: GPL-2.0

#include <linux/mtd/rawnand.h>
#include <linux/sizes.h>
#include "sp_spinand.h"

#define NAND_ID(nm, mid, did, pgsz, oobsz, bksz, chipsz) \
	{.name = (nm), {{.mfr_id = mid, .dev_id = did}}, .pagesize = pgsz, \
	.chipsize = chipsz, .erasesize = bksz, .options = 0, .id_len = 2, \
	.oobsize = oobsz,}

#define NAND_OPT(nm, mid, did, opt) \
	{.name = (nm), {{.mfr_id = mid, .dev_id = did}}, .options = opt,}

/*
 * Chip ID list
 *
 * nm     - nand chip name.
 * mid    - manufacture id.
 * did    - device id.
 * pgsz   - page size. (unit: byte)
 * oobsz  - oob size. (unit: byte)
 * bksz   - block/erase size. (unit: byte)
 * chipsz - chip size. (unit: mega byte)
 */
struct nand_flash_dev sp_spinand_ids[] = {
	/*
	 * NAND_ID(name,             mid,  did,  pgsz,  oobsz,  bksz,    chipsz)
	 */
	NAND_ID("MT29F4G01-ZEBU",    0x2c, 0x32, SZ_2K, SZ_64,  SZ_128K, SZ_512),

	/* Micron */
	NAND_ID("MT29F1G01ABADD",    0x2c, 0x14, SZ_2K, SZ_64,  SZ_128K, SZ_128),
	NAND_ID("MT29F2G01ABADD",    0x2c, 0x24, SZ_2K, SZ_64,  SZ_128K, SZ_256),
	NAND_ID("MT29F4G01ABAFD",    0x2c, 0x34, SZ_4K, SZ_256, SZ_256K, SZ_512),

	/* MXIC */
	NAND_ID("MX35LF1GE4AB",      0xc2, 0x12, SZ_2K, SZ_64,  SZ_128K, SZ_128),
	NAND_ID("MX35UF1G24AD",      0xc2, 0x94, SZ_2K, SZ_128, SZ_128K, SZ_128),
	NAND_ID("MX35LF2GE4AB",      0xc2, 0x22, SZ_2K, SZ_64,  SZ_128K, SZ_256),
	NAND_ID("MX35LF2G14AC",      0xc2, 0x20, SZ_2K, SZ_64,  SZ_128K, SZ_256),
	NAND_ID("MX35UF2G24AD",      0xc2, 0xa4, SZ_2K, SZ_128, SZ_128K, SZ_256),

	/* Etron */
	NAND_ID("EM73C044VCC-H",     0xd5, 0x22, SZ_2K, SZ_64,  SZ_128K, SZ_128),
	NAND_ID("EM73D044VCE-H",     0xd5, 0x20, SZ_2K, SZ_64,  SZ_128K, SZ_256),

	/* Winbond */
	//NAND_ID("W25N01GVxxIG",      0xef, 0xaa, SZ_2K, SZ_64,  SZ_128K, SZ_128,),
	NAND_ID("W25M02GVxxIG",      0xef, 0xab, SZ_2K, SZ_64,  SZ_128K, SZ_128),
	NAND_ID("W25N02JWxxFC",      0xef, 0xbf, SZ_2K, SZ_64,  SZ_128K, SZ_256),
	NAND_ID("W25N04KWxxIR",      0xef, 0xba, SZ_2K, SZ_64,  SZ_128K, SZ_512),
	NAND_ID("W25N04KVxxIR",      0xef, 0xaa, SZ_2K, SZ_64,  SZ_128K, SZ_512),

	/* GiGA */
	NAND_ID("GD5F1GQ4UBYIG",     0xc8, 0xd1, SZ_2K, SZ_64,  SZ_128K, SZ_128),
	NAND_ID("GD5F2GQ4UBYIG",     0xc8, 0xd2, SZ_2K, SZ_64,  SZ_128K, SZ_256),
	NAND_ID("GD5F4GQ4UBYIG",     0xc8, 0xd4, SZ_4K, SZ_256, SZ_256K, SZ_512),

	/* ESMT */
	NAND_ID("F50L1G41LB",        0xc8, 0x01, SZ_2K, SZ_64,  SZ_128K, SZ_128),
	NAND_ID("F50L2G41LB",        0xc8, 0x0a, SZ_2K, SZ_64,  SZ_128K, SZ_128),

	/* ISSI */
	NAND_ID("IS38SML01G1-LLA1",  0xc8, 0x21, SZ_2K, SZ_64,  SZ_128K, SZ_128),

	/*phison*/
	NAND_ID("CS11G0-S0A0AA",     0x6b, 0x20, SZ_2K, SZ_64,  SZ_128K, SZ_128),
	NAND_ID("CS11G1-S0A0AA",     0x6b, 0x21, SZ_2K, SZ_64,  SZ_128K, SZ_256),
	NAND_ID("CS11G2-S0A0AA",     0x6b, 0x22, SZ_2K, SZ_64,  SZ_128K, SZ_512),

	/*Toshiba*/
	NAND_ID("TC58CVG0S3H",       0x98, 0xc2, SZ_2K, SZ_128, SZ_128K, SZ_128),
	NAND_ID("TC58CVG1S3H",       0x98, 0xcb, SZ_2K, SZ_128, SZ_128K, SZ_256),
	NAND_ID("TC58CVG1S3H",       0x98, 0xcd, SZ_4K, SZ_128, SZ_256K, SZ_512),

	/* XTX */
	NAND_ID("XT26G01AWSEGA",     0x0b, 0xe1, SZ_2K, SZ_64,  SZ_128K, SZ_128),
	NAND_ID("XT26Q01DWSIG",      0x0b, 0x51, SZ_2K, SZ_64,  SZ_128K, SZ_128),
	NAND_ID("XT26G02AWSEGA",     0x0b, 0xe2, SZ_2K, SZ_64,  SZ_128K, SZ_256),
	NAND_ID("XT26Q02ELGIG",      0x2c, 0x25, SZ_2K, SZ_128, SZ_128K, SZ_256),
	NAND_ID("XT26G04DWSIG",      0x0b, 0x33, SZ_4K, SZ_64,  SZ_256K, SZ_512),

	/* FORESEE */
	NAND_ID("FS35NF01G-S1F1",    0xcd, 0xb1, SZ_2K, SZ_64,  SZ_128K, SZ_128),
	NAND_ID("FS35NF02G-S2F1",    0xcd, 0xa2, SZ_2K, SZ_64,  SZ_128K, SZ_256),
	{NULL},
};

/*
 * Chip option list
 *
 * nm     - nand chip name.
 * mid    - manufacture id.
 * did    - device id.
 * drvopt - driver option. refer to SPINAND_OPT_*
 */
struct sp_drv_option sp_spinand_opt[] = {
	/*
	 * NAND_OPT(name,            mid,  did,  drvopt)
	 */
	NAND_OPT("MT29F4G01-ZEBU",   0x2c, 0x32, SPINAND_OPT_HAS_TWO_PLANE|SPINAND_OPT_NO_4BIT_PROGRAM),

	/* Micron */
	NAND_OPT("MT29F1G01ABADD",   0x2c, 0x14, 0),
	NAND_OPT("MT29F2G01ABADD",   0x2c, 0x24, SPINAND_OPT_HAS_TWO_PLANE),
	NAND_OPT("MT29F4G01ABAFD",   0x2c, 0x34, SPINAND_OPT_HAS_CONTI_RD),

	/* MXIC */
	NAND_OPT("MX35LF1GE4AB",     0xc2, 0x12, SPINAND_OPT_HAS_QE_BIT),
	NAND_OPT("MX35UF1G24AD",     0xc2, 0x94, SPINAND_OPT_HAS_QE_BIT),
	NAND_OPT("MX35LF2GE4AB",     0xc2, 0x22, SPINAND_OPT_HAS_QE_BIT|SPINAND_OPT_HAS_TWO_PLANE),
	NAND_OPT("MX35LF2G14AC",     0xc2, 0x20, SPINAND_OPT_HAS_QE_BIT|SPINAND_OPT_HAS_TWO_PLANE),
	NAND_OPT("MX35UF2G24AD",     0xc2, 0xa4, SPINAND_OPT_HAS_QE_BIT|SPINAND_OPT_HAS_TWO_PLANE),

	/* Etron */
	NAND_OPT("EM73C044VCC-H",    0xd5, 0x22, SPINAND_OPT_HAS_QE_BIT),
	NAND_OPT("EM73D044VCE-H",    0xd5, 0x20, SPINAND_OPT_HAS_QE_BIT),

	/* Winbond */
	//NAND_OPT("W25N01GVxxIG",     0xef, 0xaa, SPINAND_OPT_HAS_BUF_BIT),
	NAND_OPT("W25M02GVxxIG",     0xef, 0xab, SPINAND_OPT_HAS_BUF_BIT|SPINAND_OPT_SET_DIENUM(2)),
	NAND_OPT("W25N02JWxxFC",     0xef, 0xbf, SPINAND_OPT_HAS_BUF_BIT),
	NAND_OPT("W25N04KWxxIR",     0xef, 0xba, SPINAND_OPT_HAS_BUF_BIT),
	NAND_OPT("W25N04KVxxIR",     0xef, 0xaa, SPINAND_OPT_HAS_BUF_BIT),

	/* GiGA */
	NAND_OPT("GD5F1GQ4UBYIG",    0xc8, 0xd1, SPINAND_OPT_HAS_QE_BIT),
	NAND_OPT("GD5F2GQ4UBYIG",    0xc8, 0xd2, SPINAND_OPT_HAS_QE_BIT),
	NAND_OPT("GD5F4GQ4UBYIG",    0xc8, 0xd4, SPINAND_OPT_HAS_QE_BIT),

	/* ESMT */
	NAND_OPT("F50L1G41LB",       0xc8, 0x01, 0),
	NAND_OPT("F50L2G41LB",       0xc8, 0x0a, SPINAND_OPT_SET_DIENUM(2)),

	/* ISSI */
	NAND_OPT("IS38SML01G1-LLA1", 0xc8, 0x21, 0),

	/*phison*/
	NAND_OPT("CS11G0-S0A0AA",    0x6b, 0x20, SPINAND_OPT_HAS_QE_BIT),
	NAND_OPT("CS11G1-S0A0AA",    0x6b, 0x21, SPINAND_OPT_HAS_QE_BIT),
	NAND_OPT("CS11G2-S0A0AA",    0x6b, 0x22, SPINAND_OPT_HAS_QE_BIT),

	/*Toshiba*/
	NAND_OPT("TC58CVG0S3H",      0x98, 0xc2, SPINAND_OPT_NO_4BIT_PROGRAM),
	NAND_OPT("TC58CVG1S3H",      0x98, 0xcb, SPINAND_OPT_NO_4BIT_PROGRAM),
	NAND_OPT("TC58CVG1S3H",      0x98, 0xcd, SPINAND_OPT_NO_4BIT_PROGRAM),

	/* XTX */
	NAND_OPT("XT26G01AWSEGA",    0x0b, 0xe1, SPINAND_OPT_HAS_QE_BIT),
	NAND_OPT("XT26Q01DWSIG",     0x0b, 0x51, SPINAND_OPT_HAS_QE_BIT),
	NAND_OPT("XT26G02AWSEGA",    0x0b, 0xe2, SPINAND_OPT_HAS_QE_BIT),
	NAND_OPT("XT26Q02ELGIG",     0x2c, 0x25, SPINAND_OPT_HAS_QE_BIT|SPINAND_OPT_HAS_TWO_PLANE),
	NAND_OPT("XT26G04DWSIG",     0x0b, 0x33, SPINAND_OPT_HAS_QE_BIT),

	/* FORESEE */
	NAND_OPT("FS35NF01G-S1F1",   0xcd, 0xb1, SPINAND_OPT_ECCEN_IN_F90_4|SPINAND_OPT_HAS_QE_BIT),
	NAND_OPT("FS35NF02G-S2F1",   0xcd, 0xa2, SPINAND_OPT_ECCEN_IN_F90_4|SPINAND_OPT_NO_4BIT_READ|SPINAND_OPT_NO_4BIT_PROGRAM),
	{NULL},
};

