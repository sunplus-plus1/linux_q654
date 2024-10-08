// SPDX-License-Identifier: GPL-2.0

#include <linux/seq_file.h>
#include <linux/io.h>
#include "gpio-sp7350.h"

#define REG_OFFSET_CTL 0x00 /* gpioxt_regs_base */
#define REG_OFFSET_OUTPUT_ENABLE 0x34 /* gpioxt_regs_base */
#define REG_OFFSET_OUTPUT 0x68 /* gpioxt_regs_base */
#define REG_OFFSET_INPUT 0x9c /* gpioxt_regs_base */
#define REG_OFFSET_INPUT_INVERT 0xbc /* gpioxt_regs_base */
#define REG_OFFSET_OUTPUT_INVERT 0xf0 /* gpioxt_regs_base */
#define REG_OFFSET_OPEN_DRAIN 0x124 /* gpioxt_regs_base */

#define REG_OFFSET_GPIO_FIRST 0x00 /* first_regs_base */

#define REG_OFFSET_SCHMITT_TRIGGER 0x00 /* padctl1_regs_base */
#define REG_OFFSET_DS0 0x10 /* padctl1_regs_base */
#define REG_OFFSET_DS1 0x20 /* padctl1_regs_base */
#define REG_OFFSET_DS2 0x30 /* padctl1_regs_base */
#define REG_OFFSET_DS3 0x40 /* padctl1_regs_base */

#define REG_OFFSET_SLEW_RATE 0x00 /* padctl2_regs_base */
#define REG_OFFSET_PULL_ENABLE 0x08 /* padctl2_regs_base */
#define REG_OFFSET_PULL_SELECTOR 0x10 /* padctl2_regs_base */
#define REG_OFFSET_STRONG_PULL_UP 0x18 /* padctl2_regs_base */
#define REG_OFFSET_PULL_UP 0x20 /* padctl2_regs_base */
#define REG_OFFSET_PULL_DOWN 0x28 /* padctl2_regs_base */
#define REG_OFFSET_MODE_SELECT 0x30 /* padctl2_regs_base */

// (/16)*4
#define R16_ROF(r) (((r) >> 4) << 2)
#define R16_BOF(r) ((r) % 16)
// (/32)*4
#define R32_ROF(r) (((r) >> 5) << 2)
#define R32_BOF(r) ((r) % 32)
#define R32_VAL(r, boff) (((r) >> (boff)) & BIT(0))
//(/30)*4
#define R30_ROF(r) (((r) / 30) << 2)
#define R30_BOF(r) ((r) % 30)
#define R30_VAL(r, boff) (((r) >> (boff)) & BIT(0))

#define IS_DVIO(pin) ((pin) >= 20 && (pin) <= 79)

const char *const sppctlgpio_list_s[] = {
	D_PIS(0),   D_PIS(1),	D_PIS(2),   D_PIS(3),	D_PIS(4),   D_PIS(5),
	D_PIS(6),   D_PIS(7),	D_PIS(8),   D_PIS(9),	D_PIS(10),  D_PIS(11),
	D_PIS(12),  D_PIS(13),	D_PIS(14),  D_PIS(15),	D_PIS(16),  D_PIS(17),
	D_PIS(18),  D_PIS(19),	D_PIS(20),  D_PIS(21),	D_PIS(22),  D_PIS(23),
	D_PIS(24),  D_PIS(25),	D_PIS(26),  D_PIS(27),	D_PIS(28),  D_PIS(29),
	D_PIS(30),  D_PIS(31),	D_PIS(32),  D_PIS(33),	D_PIS(34),  D_PIS(35),
	D_PIS(36),  D_PIS(37),	D_PIS(38),  D_PIS(39),	D_PIS(40),  D_PIS(41),
	D_PIS(42),  D_PIS(43),	D_PIS(44),  D_PIS(45),	D_PIS(46),  D_PIS(47),
	D_PIS(48),  D_PIS(49),	D_PIS(50),  D_PIS(51),	D_PIS(52),  D_PIS(53),
	D_PIS(54),  D_PIS(55),	D_PIS(56),  D_PIS(57),	D_PIS(58),  D_PIS(59),
	D_PIS(60),  D_PIS(61),	D_PIS(62),  D_PIS(63),	D_PIS(64),  D_PIS(65),
	D_PIS(66),  D_PIS(67),	D_PIS(68),  D_PIS(69),	D_PIS(70),  D_PIS(71),
	D_PIS(72),  D_PIS(73),	D_PIS(74),  D_PIS(75),	D_PIS(76),  D_PIS(77),
	D_PIS(78),  D_PIS(79),	D_PIS(80),  D_PIS(81),	D_PIS(82),  D_PIS(83),
	D_PIS(84),  D_PIS(85),	D_PIS(86),  D_PIS(87),	D_PIS(88),  D_PIS(89),
	D_PIS(90),  D_PIS(91),	D_PIS(92),  D_PIS(93),	D_PIS(94),  D_PIS(95),
	D_PIS(96),  D_PIS(97),	D_PIS(98),  D_PIS(99),	D_PIS(100), D_PIS(101),
	D_PIS(102), D_PIS(103), D_PIS(104), D_PIS(105),
};

const size_t GPIS_list_size =
	sizeof(sppctlgpio_list_s) / sizeof(*(sppctlgpio_list_s));

// who is first: GPIO(1) | MUX(0)
int sppctl_gpio_first_get(struct gpio_chip *chip, unsigned int selector)
{
	struct sppctlgpio_chip_t *pc;
	u32 r;

	pc = (struct sppctlgpio_chip_t *)gpiochip_get_data(chip);

	r = readl(pc->first_regs_base + REG_OFFSET_GPIO_FIRST +
		  R32_ROF(selector));
	//KINF(chip->parent, "u F r:%X = %d %px off:%d\n", r, R32_VAL(r,R32_BOF(selector)),
	//pc->padctl1_regs_base, REG_OFFSET_GPIO_FIRST + R32_ROF(selector));

	return R32_VAL(r, R32_BOF(selector));
}

// who is master: GPIO(1) | IOP(0)
int sppctl_gpio_master_get(struct gpio_chip *chip, unsigned int selector)
{
	struct sppctlgpio_chip_t *pc;
	u32 r;

	pc = (struct sppctlgpio_chip_t *)gpiochip_get_data(chip);

	r = readl(pc->gpioxt_regs_base + REG_OFFSET_CTL + R16_ROF(selector));

	//KINF(chip->parent, "u M r:%X = %d %px off:%d\n", r, R32_VAL(r,R16_BOF(selector)),
	//pc->gpioxt_regs_base, REG_OFFSET_CTL + R16_ROF(selector));

	return R32_VAL(r, R16_BOF(selector));
}

// set master: GPIO(1)|IOP(0), first:GPIO(1)|MUX(0)
void sppctl_gpio_first_master_set(struct gpio_chip *chip, unsigned int selector,
				  enum MUX_FIRST_MG_t first_sel,
				  enum MUX_MASTER_IG_t master_sel)
{
	struct sppctlgpio_chip_t *pc;
	u32 r;

	pc = (struct sppctlgpio_chip_t *)gpiochip_get_data(chip);

	// FIRST
	if (first_sel != MUX_FIRST_KEEP) {
		r = readl(pc->first_regs_base + REG_OFFSET_GPIO_FIRST +
			  R32_ROF(selector));
		//KINF(chip->parent, "F r:%X %px off:%d\n", r, pc->padctl1_regs_base,
		//	REG_OFFSET_GPIO_FIRST + R32_ROF(selector));
		if (first_sel != R32_VAL(r, R32_BOF(selector))) {
			if (first_sel == MUX_FIRST_G)
				r |= BIT(R32_BOF(selector));
			else
				r &= ~BIT(R32_BOF(selector));
			//KINF(chip->parent, "F w:%X\n", r);
			writel(r, pc->first_regs_base + REG_OFFSET_GPIO_FIRST +
					  R32_ROF(selector));
		}
	}

	// MASTER
	if (master_sel != MUX_MASTER_KEEP) {
		r = (BIT(R16_BOF(selector)) << 16);
		if (master_sel == MUX_MASTER_G)
			r |= BIT(R16_BOF(selector));
		//KINF(chip->parent, "M w:%X %px off:%d\n", r, pc->gpioxt_regs_base,
		//	REG_OFFSET_CTL + R16_ROF(selector));
		writel(r, pc->gpioxt_regs_base + REG_OFFSET_CTL +
				  R16_ROF(selector));
	}
}

int sppctl_gpio_output_invert_query(struct gpio_chip *chip,
				    unsigned int selector)
{
	struct sppctlgpio_chip_t *pc;
	u32 r;

	pc = (struct sppctlgpio_chip_t *)gpiochip_get_data(chip);

	r = readl(pc->gpioxt_regs_base + REG_OFFSET_OUTPUT_INVERT +
		  R16_ROF(selector));

	return R32_VAL(r, R16_BOF(selector));
}

int sppctl_gpio_input_invert_query(struct gpio_chip *chip,
				   unsigned int selector)
{
	struct sppctlgpio_chip_t *pc;
	u32 r;

	pc = (struct sppctlgpio_chip_t *)gpiochip_get_data(chip);

	r = readl(pc->gpioxt_regs_base + REG_OFFSET_INPUT_INVERT +
		  R16_ROF(selector));

	return R32_VAL(r, R16_BOF(selector));
}

int sppctl_gpio_is_inverted(struct gpio_chip *chip, unsigned int selector)
{
	if (sppctl_gpio_output_enable_query(chip, selector))
		return sppctl_gpio_output_invert_query(chip, selector);
	else
		return sppctl_gpio_input_invert_query(chip, selector);
}

void sppctl_gpio_input_invert_set(struct gpio_chip *chip, unsigned int selector,
				  unsigned int value)
{
	struct sppctlgpio_chip_t *pc;
	u32 r;

	pc = (struct sppctlgpio_chip_t *)gpiochip_get_data(chip);

	r = (BIT(R16_BOF(selector)) << 16) |
	    ((value & BIT(0)) << R16_BOF(selector));
	writel(r, pc->gpioxt_regs_base + REG_OFFSET_INPUT_INVERT +
			  R16_ROF(selector));
}

void sppctl_gpio_output_invert_set(struct gpio_chip *chip,
				   unsigned int selector, unsigned int value)
{
	struct sppctlgpio_chip_t *pc;
	u32 r;

	pc = (struct sppctlgpio_chip_t *)gpiochip_get_data(chip);

	r = (BIT(R16_BOF(selector)) << 16) |
	    ((value & BIT(0)) << R16_BOF(selector));
	writel(r, pc->gpioxt_regs_base + REG_OFFSET_OUTPUT_INVERT +
			  R16_ROF(selector));
}

// is open-drain: YES(1) | NON(0)
int sppctl_gpio_open_drain_mode_query(struct gpio_chip *chip,
				      unsigned int selector)
{
	struct sppctlgpio_chip_t *pc;
	u32 r;

	pc = (struct sppctlgpio_chip_t *)gpiochip_get_data(chip);

	r = readl(pc->gpioxt_regs_base + REG_OFFSET_OPEN_DRAIN +
		  R16_ROF(selector));

	return R32_VAL(r, R16_BOF(selector));
}

void sppctl_gpio_open_drain_mode_set(struct gpio_chip *chip,
				     unsigned int selector, unsigned int value)
{
	struct sppctlgpio_chip_t *pc;
	u32 r;

	pc = (struct sppctlgpio_chip_t *)gpiochip_get_data(chip);

	r = (BIT(R16_BOF(selector)) << 16) |
	    ((value & BIT(0)) << R16_BOF(selector));
	writel(r, pc->gpioxt_regs_base + REG_OFFSET_OPEN_DRAIN +
			  R16_ROF(selector));
}

/* enable/disable schmitt trigger */
int sppctl_gpio_schmitt_trigger_set(struct gpio_chip *chip,
				    unsigned int selector, int value)
{
	struct sppctlgpio_chip_t *pc;
	u32 r;

	pc = (struct sppctlgpio_chip_t *)gpiochip_get_data(chip);

	if (value < 0)
		return -EINVAL;

	r = readl(pc->padctl1_regs_base + REG_OFFSET_SCHMITT_TRIGGER +
		  R32_ROF(selector));
	if (value == 0)
		r &= ~BIT(R32_BOF(selector));
	else
		r |= BIT(R32_BOF(selector));

	writel(r, pc->padctl1_regs_base + REG_OFFSET_SCHMITT_TRIGGER +
			  R32_ROF(selector));

	return 0;
}

/* enable/disable schmitt trigger */
int sppctl_gpio_schmitt_trigger_query(struct gpio_chip *chip,
				      unsigned int selector)
{
	struct sppctlgpio_chip_t *pc;
	u32 r;

	pc = (struct sppctlgpio_chip_t *)gpiochip_get_data(chip);

	r = readl(pc->padctl1_regs_base + REG_OFFSET_SCHMITT_TRIGGER +
		  R32_ROF(selector));

	return R32_VAL(r, R32_BOF(selector));
}

/* slew-rate control; for GPIO only */
int sppctl_gpio_slew_rate_control_set(struct gpio_chip *chip,
				      unsigned int selector, unsigned int value)
{
	struct sppctlgpio_chip_t *pc;
	unsigned int pin;
	u32 r;

	pc = (struct sppctlgpio_chip_t *)gpiochip_get_data(chip);

	if (IS_DVIO(selector))
		return -EINVAL;

	pin = (selector > 19) ? selector - 60 : selector;

	r = readl(pc->padctl2_regs_base + REG_OFFSET_SLEW_RATE + R32_ROF(pin));

	if (value)
		r |= BIT(R32_BOF(pin));
	else
		r &= ~BIT(R32_BOF(pin));

	writel(r, pc->padctl2_regs_base + REG_OFFSET_SLEW_RATE + R32_ROF(pin));

	return 0;
}

/* slew-rate control; for GPIO only */
int sppctl_gpio_slew_rate_control_query(struct gpio_chip *chip,
					unsigned int selector)
{
	struct sppctlgpio_chip_t *pc;
	unsigned int pin;
	u32 r;

	pc = (struct sppctlgpio_chip_t *)gpiochip_get_data(chip);

	if (IS_DVIO(selector))
		return -EINVAL;

	pin = (selector > 19) ? selector - 60 : selector;

	r = readl(pc->padctl2_regs_base + REG_OFFSET_SLEW_RATE + R32_ROF(pin));

	return R32_VAL(r, R32_BOF(selector));
}

/* pull-up */
int sppctl_gpio_pull_up(struct gpio_chip *chip, unsigned int selector)
{
	struct sppctlgpio_chip_t *pc;
	unsigned int pin;
	u32 r;

	pc = (struct sppctlgpio_chip_t *)gpiochip_get_data(chip);

	if (IS_DVIO(selector)) {
		pin = selector - 20;

		/* PU=1 */
		r = readl(pc->padctl2_regs_base + REG_OFFSET_PULL_UP +
			  R30_ROF(pin));
		r |= BIT(R30_BOF(pin));
		writel(r, pc->padctl2_regs_base + REG_OFFSET_PULL_UP +
				  R30_ROF(pin));

		/* PD=0 */
		r = readl(pc->padctl2_regs_base + REG_OFFSET_PULL_DOWN +
			  R30_ROF(pin));

		r &= ~BIT(R30_BOF(pin));
		writel(r, pc->padctl2_regs_base + REG_OFFSET_PULL_DOWN +
				  R30_ROF(pin));

	} else {
		pin = (selector > 19) ? selector - 60 : selector;

		/* PE=1 */
		r = readl(pc->padctl2_regs_base + REG_OFFSET_PULL_ENABLE +
			  R32_ROF(pin));

		r |= BIT(R32_BOF(pin));
		writel(r, pc->padctl2_regs_base + REG_OFFSET_PULL_ENABLE +
				  R32_ROF(pin));

		/* PS=1 */
		r = readl(pc->padctl2_regs_base + REG_OFFSET_PULL_SELECTOR +
			  R32_ROF(pin));

		r |= BIT(R32_BOF(pin));
		writel(r, pc->padctl2_regs_base + REG_OFFSET_PULL_SELECTOR +
				  R32_ROF(pin));

		/* SPU=0 */
		r = readl(pc->padctl2_regs_base + REG_OFFSET_STRONG_PULL_UP +
			  R32_ROF(pin));

		r &= ~BIT(R32_BOF(pin));
		writel(r, pc->padctl2_regs_base + REG_OFFSET_STRONG_PULL_UP +
				  R32_ROF(pin));
	}

	return 0;
}

/* pull-up */
int sppctl_gpio_pull_up_query(struct gpio_chip *chip, unsigned int selector)
{
	struct sppctlgpio_chip_t *pc;
	unsigned int pin;
	u32 pu;
	u32 pd;
	u32 pe;
	u32 ps;
	u32 spu;
	u32 r;

	pc = (struct sppctlgpio_chip_t *)gpiochip_get_data(chip);

	if (IS_DVIO(selector)) {
		pin = selector - 20;

		/* PU*/
		r = readl(pc->padctl2_regs_base + REG_OFFSET_PULL_UP +
			  R30_ROF(pin));
		pu = R30_VAL(r, R30_BOF(pin));

		/* PD*/
		r = readl(pc->padctl2_regs_base + REG_OFFSET_PULL_DOWN +
			  R30_ROF(pin));
		pd = R30_VAL(r, R30_BOF(pin));

		if (pu == 1 && pd == 0)
			return 1;
		else
			return 0;

	} else {
		pin = (selector > 19) ? selector - 60 : selector;

		/* PE*/
		r = readl(pc->padctl2_regs_base + REG_OFFSET_PULL_ENABLE +
			  R32_ROF(pin));
		pe = R32_VAL(r, R32_BOF(pin));

		/* PS*/
		r = readl(pc->padctl2_regs_base + REG_OFFSET_PULL_SELECTOR +
			  R32_ROF(pin));
		ps = R32_VAL(r, R32_BOF(pin));

		/* SPU*/
		r = readl(pc->padctl2_regs_base + REG_OFFSET_STRONG_PULL_UP +
			  R32_ROF(pin));
		spu = R32_VAL(r, R32_BOF(pin));

		if (pe == 1 && ps == 1 && spu == 0)
			return 1;
		else
			return 0;
	}
}

/* pull-down */
int sppctl_gpio_pull_down(struct gpio_chip *chip, unsigned int selector)
{
	struct sppctlgpio_chip_t *pc;
	u32 r;

	pc = (struct sppctlgpio_chip_t *)gpiochip_get_data(chip);

	if (IS_DVIO(selector)) {
		unsigned int pin = selector - 20;

		/* PU=0 */
		r = readl(pc->padctl2_regs_base + REG_OFFSET_PULL_UP +
			  R30_ROF(pin));

		r &= ~BIT(R30_BOF(pin));
		writel(r, pc->padctl2_regs_base + REG_OFFSET_PULL_UP +
				  R30_ROF(pin));

		/* PD=1 */
		r = readl(pc->padctl2_regs_base + REG_OFFSET_PULL_DOWN +
			  R30_ROF(pin));
		r |= BIT(R30_BOF(pin));
		writel(r, pc->padctl2_regs_base + REG_OFFSET_PULL_DOWN +
				  R30_ROF(pin));

	} else {
		unsigned int pin = (selector > 19) ? selector - 60 : selector;
		/* PE=1 */
		r = readl(pc->padctl2_regs_base + REG_OFFSET_PULL_ENABLE +
			  R32_ROF(pin));
		r |= BIT(R32_BOF(pin));
		writel(r, pc->padctl2_regs_base + REG_OFFSET_PULL_ENABLE +
				  R32_ROF(pin));

		/* PS=0 */
		r = readl(pc->padctl2_regs_base + REG_OFFSET_PULL_SELECTOR +
			  R32_ROF(pin));
		r &= ~BIT(R32_BOF(pin));
		writel(r, pc->padctl2_regs_base + REG_OFFSET_PULL_SELECTOR +
				  R32_ROF(pin));

		/* SPU=0 */
		r = readl(pc->padctl2_regs_base + REG_OFFSET_STRONG_PULL_UP +
			  R32_ROF(pin));
		r &= ~BIT(R32_BOF(pin));
		writel(r, pc->padctl2_regs_base + REG_OFFSET_STRONG_PULL_UP +
				  R32_ROF(pin));
	}
	return 0;
}

int sppctl_gpio_pull_down_query(struct gpio_chip *chip, unsigned int selector)
{
	struct sppctlgpio_chip_t *pc;
	unsigned int pin;
	u32 pu;
	u32 pd;
	u32 pe;
	u32 ps;
	u32 spu;
	u32 r;

	pc = (struct sppctlgpio_chip_t *)gpiochip_get_data(chip);

	if (IS_DVIO(selector)) {
		pin = selector - 20;

		/* PU*/
		r = readl(pc->padctl2_regs_base + REG_OFFSET_PULL_UP +
			  R30_ROF(pin));
		pu = R30_VAL(r, R30_BOF(pin));

		/* PD*/
		r = readl(pc->padctl2_regs_base + REG_OFFSET_PULL_DOWN +
			  R30_ROF(pin));
		pd = R30_VAL(r, R30_BOF(pin));

		if (pu == 0 && pd == 1)
			return 1;
		else
			return 0;

	} else {
		pin = (selector > 19) ? selector - 60 : selector;

		/* PE*/
		r = readl(pc->padctl2_regs_base + REG_OFFSET_PULL_ENABLE +
			  R32_ROF(pin));
		pe = R32_VAL(r, R32_BOF(pin));

		/* PS*/
		r = readl(pc->padctl2_regs_base + REG_OFFSET_PULL_SELECTOR +
			  R32_ROF(pin));
		ps = R32_VAL(r, R32_BOF(pin));

		/* SPU*/
		r = readl(pc->padctl2_regs_base + REG_OFFSET_STRONG_PULL_UP +
			  R32_ROF(pin));
		spu = R32_VAL(r, R32_BOF(pin));

		if (pe == 1 && ps == 0 && spu == 0)
			return 1;
		else
			return 0;
	}
}

/* strongly pull-up; for GPIO only */
int sppctl_gpio_strong_pull_up(struct gpio_chip *chip, unsigned int selector)
{
	struct sppctlgpio_chip_t *pc;
	unsigned int pin;
	u32 r;

	pc = (struct sppctlgpio_chip_t *)gpiochip_get_data(chip);

	if (IS_DVIO(selector))
		return -EINVAL;

	pin = (selector > 19) ? selector - 60 : selector;
	/* PE=1 */
	r = readl(pc->padctl2_regs_base + REG_OFFSET_PULL_ENABLE +
		  R32_ROF(pin));
	r |= BIT(R32_BOF(pin));
	writel(r,
	       pc->padctl2_regs_base + REG_OFFSET_PULL_ENABLE + R32_ROF(pin));

	/* PS=1 */
	r = readl(pc->padctl2_regs_base + REG_OFFSET_PULL_SELECTOR +
		  R32_ROF(pin));
	r |= BIT(R32_BOF(pin));
	writel(r,
	       pc->padctl2_regs_base + REG_OFFSET_PULL_SELECTOR + R32_ROF(pin));

	/* SPU=1 */
	r = readl(pc->padctl2_regs_base + REG_OFFSET_STRONG_PULL_UP +
		  R32_ROF(pin));
	r |= BIT(R32_BOF(pin));
	writel(r, pc->padctl2_regs_base + REG_OFFSET_STRONG_PULL_UP +
			  R32_ROF(pin));

	return 0;
}

int sppctl_gpio_strong_pull_up_query(struct gpio_chip *chip,
				     unsigned int selector)
{
	struct sppctlgpio_chip_t *pc;
	unsigned int pin;
	u32 pe;
	u32 ps;
	u32 spu;
	u32 r;

	pc = (struct sppctlgpio_chip_t *)gpiochip_get_data(chip);

	if (IS_DVIO(selector))
		return -EINVAL;

	pin = (selector > 19) ? selector - 60 : selector;

	/* PE*/
	r = readl(pc->padctl2_regs_base + REG_OFFSET_PULL_ENABLE +
		  R32_ROF(pin));
	pe = R32_VAL(r, R32_BOF(pin));

	/* PS*/
	r = readl(pc->padctl2_regs_base + REG_OFFSET_PULL_SELECTOR +
		  R32_ROF(pin));
	ps = R32_VAL(r, R32_BOF(pin));

	/* SPU*/
	r = readl(pc->padctl2_regs_base + REG_OFFSET_STRONG_PULL_UP +
		  R32_ROF(pin));
	spu = R32_VAL(r, R32_BOF(pin));

	if (pe == 1 && ps == 1 && spu == 1)
		return 1;
	else
		return 0;
}

/* high-Z; */
int sppctl_gpio_high_impedance(struct gpio_chip *chip, unsigned int selector)
{
	struct sppctlgpio_chip_t *pc;
	unsigned int pin;
	u32 r;

	pc = (struct sppctlgpio_chip_t *)gpiochip_get_data(chip);

	if (!IS_DVIO(selector)) {
		pin = (selector > 19) ? selector - 60 : selector;
		/* PE=0 */
		r = readl(pc->padctl2_regs_base + REG_OFFSET_PULL_ENABLE +
			  R32_ROF(pin));
		r &= ~BIT(R32_BOF(pin));
		writel(r, pc->padctl2_regs_base + REG_OFFSET_PULL_ENABLE +
				  R32_ROF(pin));

		/* PS=0 */
		r = readl(pc->padctl2_regs_base + REG_OFFSET_PULL_SELECTOR +
			  R32_ROF(pin));
		r &= ~BIT(R32_BOF(pin));
		writel(r, pc->padctl2_regs_base + REG_OFFSET_PULL_SELECTOR +
				  R32_ROF(pin));

		/* SPU=0 */
		r = readl(pc->padctl2_regs_base + REG_OFFSET_STRONG_PULL_UP +
			  R32_ROF(pin));
		r &= ~BIT(R32_BOF(pin));
		writel(r, pc->padctl2_regs_base + REG_OFFSET_STRONG_PULL_UP +
				  R32_ROF(pin));
	} else {
		pin = selector - 20;

		/* PU=0 */
		r = readl(pc->padctl2_regs_base + REG_OFFSET_PULL_UP +
			  R30_ROF(pin));
		r &= ~BIT(R30_BOF(pin));
		writel(r, pc->padctl2_regs_base + REG_OFFSET_PULL_UP +
				  R30_ROF(pin));

		/* PD=0 */
		r = readl(pc->padctl2_regs_base + REG_OFFSET_PULL_DOWN +
			  R30_ROF(pin));
		r &= ~BIT(R30_BOF(pin));
		writel(r, pc->padctl2_regs_base + REG_OFFSET_PULL_DOWN +
				  R30_ROF(pin));
	}
	return 0;
}

int sppctl_gpio_high_impedance_query(struct gpio_chip *chip,
				     unsigned int selector)
{
	struct sppctlgpio_chip_t *pc;
	unsigned int pin;
	u32 pu;
	u32 pd;
	u32 pe;
	u32 ps;
	u32 spu;
	u32 r;

	pc = (struct sppctlgpio_chip_t *)gpiochip_get_data(chip);

	if (IS_DVIO(selector)) {
		pin = selector - 20;

		/* PU*/
		r = readl(pc->padctl2_regs_base + REG_OFFSET_PULL_UP +
			  R30_ROF(pin));
		pu = R30_VAL(r, R30_BOF(pin));

		/* PD*/
		r = readl(pc->padctl2_regs_base + REG_OFFSET_PULL_DOWN +
			  R30_ROF(pin));
		pd = R30_VAL(r, R30_BOF(pin));

		if (pu == 0 && pd == 0)
			return 1;
		else
			return 0;

	} else {
		pin = (selector > 19) ? selector - 60 : selector;

		/* PE*/
		r = readl(pc->padctl2_regs_base + REG_OFFSET_PULL_ENABLE +
			  R32_ROF(pin));
		pe = R32_VAL(r, R32_BOF(pin));

		/* PS*/
		r = readl(pc->padctl2_regs_base + REG_OFFSET_PULL_SELECTOR +
			  R32_ROF(pin));
		ps = R32_VAL(r, R32_BOF(pin));

		/* SPU*/
		r = readl(pc->padctl2_regs_base + REG_OFFSET_STRONG_PULL_UP +
			  R32_ROF(pin));
		spu = R32_VAL(r, R32_BOF(pin));

		if (pe == 0 && ps == 0 && spu == 0)
			return 1;
		else
			return 0;
	}
}

/* bias disable */
int sppctl_gpio_bias_disable(struct gpio_chip *chip, unsigned int selector)
{
	struct sppctlgpio_chip_t *pc;
	unsigned int pin;
	u32 r;

	pc = (struct sppctlgpio_chip_t *)gpiochip_get_data(chip);

	if (IS_DVIO(selector)) {
		pin = selector - 20;

		/* PU=0 */
		r = readl(pc->padctl2_regs_base + REG_OFFSET_PULL_UP +
			  R30_ROF(pin));
		r &= ~BIT(R30_BOF(pin));
		writel(r, pc->padctl2_regs_base + REG_OFFSET_PULL_UP +
				  R30_ROF(pin));

		/* PD=0 */
		r = readl(pc->padctl2_regs_base + REG_OFFSET_PULL_DOWN +
			  R30_ROF(pin));
		r &= ~BIT(R30_BOF(pin));
		writel(r, pc->padctl2_regs_base + REG_OFFSET_PULL_DOWN +
				  R30_ROF(pin));
	} else {
		pin = (selector > 19) ? selector - 60 : selector;

		/* PE=0 */
		r = readl(pc->padctl2_regs_base + REG_OFFSET_PULL_ENABLE +
			  R32_ROF(pin));
		r &= ~BIT(R32_BOF(pin));
		writel(r, pc->padctl2_regs_base + REG_OFFSET_PULL_ENABLE +
				  R32_ROF(pin));

		/* PS=0 */
		r = readl(pc->padctl2_regs_base + REG_OFFSET_PULL_SELECTOR +
			  R32_ROF(pin));
		r &= ~BIT(R32_BOF(pin));
		writel(r, pc->padctl2_regs_base + REG_OFFSET_PULL_SELECTOR +
				  R32_ROF(pin));

		/* SPU=0 */
		r = readl(pc->padctl2_regs_base + REG_OFFSET_STRONG_PULL_UP +
			  R32_ROF(pin));
		r &= ~BIT(R32_BOF(pin));
		writel(r, pc->padctl2_regs_base + REG_OFFSET_STRONG_PULL_UP +
				  R32_ROF(pin));
	}
	return 0;
}

int sppctl_gpio_bias_disable_query(struct gpio_chip *chip,
				   unsigned int selector)
{
	struct sppctlgpio_chip_t *pc;
	unsigned int pin;
	u32 pu;
	u32 pd;
	u32 pe;
	u32 ps;
	u32 spu;
	u32 r;

	pc = (struct sppctlgpio_chip_t *)gpiochip_get_data(chip);

	if (IS_DVIO(selector)) {
		pin = selector - 20;

		/* PU*/
		r = readl(pc->padctl2_regs_base + REG_OFFSET_PULL_UP +
			  R30_ROF(pin));
		pu = R30_VAL(r, R30_BOF(pin));

		/* PD*/
		r = readl(pc->padctl2_regs_base + REG_OFFSET_PULL_DOWN +
			  R30_ROF(pin));
		pd = R30_VAL(r, R30_BOF(pin));

		if (pu == 0 && pd == 0)
			return 1;
		else
			return 0;

	} else {
		pin = (selector > 19) ? selector - 60 : selector;

		/* PE*/
		r = readl(pc->padctl2_regs_base + REG_OFFSET_PULL_ENABLE +
			  R32_ROF(pin));
		pe = R32_VAL(r, R32_BOF(pin));

		/* PS*/
		r = readl(pc->padctl2_regs_base + REG_OFFSET_PULL_SELECTOR +
			  R32_ROF(pin));
		ps = R32_VAL(r, R32_BOF(pin));

		/* SPU*/
		r = readl(pc->padctl2_regs_base + REG_OFFSET_STRONG_PULL_UP +
			  R32_ROF(pin));
		spu = R32_VAL(r, R32_BOF(pin));

		if (pe == 0 && ps == 0 && spu == 0)
			return 1;
		else
			return 0;
	}
}

//voltage mode select
int sppctl_gpio_voltage_mode_select_set(struct gpio_chip *chip,
					enum vol_ms_group ms_group,
					unsigned int value)
{
	struct sppctlgpio_chip_t *pc;
	unsigned int bit;
	u32 r;

	pc = (struct sppctlgpio_chip_t *)gpiochip_get_data(chip);

	switch (ms_group) {
	case G_MX_MS_TOP_0:
		bit = 0;
		break;
	case G_MX_MS_TOP_1:
		bit = 1;
		break;
	case AO_MX_MS_TOP_0:
		bit = 2;
		break;
	case AO_MX_MS_TOP_1:
		bit = 3;
		break;
	case AO_MX_MS_TOP_2:
		bit = 4;
		break;
	default:
		return -EINVAL;
	}

	r = readl(pc->padctl2_regs_base + REG_OFFSET_MODE_SELECT);

	if (value)
		r |= BIT(bit);
	else
		r &= ~BIT(bit);

	writel(r, pc->padctl2_regs_base + REG_OFFSET_MODE_SELECT);

	return 0;
}

// set driving strength in uA
int sppctl_gpio_drive_strength_set(struct gpio_chip *chip,
				   unsigned int selector, int value)
{
	struct sppctlgpio_chip_t *pc;
	int ret = 0;
	u32 ds0 = 0;
	u32 ds1 = 0;
	u32 ds2 = 0;
	u32 ds3 = 0;
	u32 r;

	pc = (struct sppctlgpio_chip_t *)gpiochip_get_data(chip);

	if (IS_DVIO(selector)) {
		switch (value) {
		case SPPCTRL_DVIO_DRV_IOH_5100_IOL_6200UA:
			ds3 = 0;
			ds2 = 0;
			ds1 = 0;
			ds0 = 0;
			break;
		case SPPCTRL_DVIO_DRV_IOH_7600_IOL_9300UA:
			ds3 = 0;
			ds2 = 0;
			ds1 = 0;
			ds0 = 1;
			break;
		case SPPCTRL_DVIO_DRV_IOH_10100_IOL_12500UA:
			ds3 = 0;
			ds2 = 0;
			ds1 = 1;
			ds0 = 0;
			break;
		case SPPCTRL_DVIO_DRV_IOH_12600_IOL_15600UA:
			ds3 = 0;
			ds2 = 0;
			ds1 = 1;
			ds0 = 1;
			break;
		case SPPCTRL_DVIO_DRV_IOH_15200_IOL_18700UA:
			ds3 = 0;
			ds2 = 1;
			ds1 = 0;
			ds0 = 0;
			break;
		case SPPCTRL_DVIO_DRV_IOH_17700_IOL_21800UA:
			ds3 = 0;
			ds2 = 1;
			ds1 = 0;
			ds0 = 1;
			break;
		case SPPCTRL_DVIO_DRV_IOH_20200_IOL_24900UA:
			ds3 = 0;
			ds2 = 1;
			ds1 = 1;
			ds0 = 0;
			break;
		case SPPCTRL_DVIO_DRV_IOH_22700_IOL_27900UA:
			ds3 = 0;
			ds2 = 1;
			ds1 = 1;
			ds0 = 1;
			break;
		case SPPCTRL_DVIO_DRV_IOH_25200_IOL_31000UA:
			ds3 = 1;
			ds2 = 0;
			ds1 = 0;
			ds0 = 0;
			break;
		case SPPCTRL_DVIO_DRV_IOH_27700_IOL_34100UA:
			ds3 = 1;
			ds2 = 0;
			ds1 = 0;
			ds0 = 1;
			break;
		case SPPCTRL_DVIO_DRV_IOH_30300_IOL_37200UA:
			ds3 = 1;
			ds2 = 0;
			ds1 = 1;
			ds0 = 0;
			break;
		case SPPCTRL_DVIO_DRV_IOH_32800_IOL_40300UA:
			ds3 = 1;
			ds2 = 0;
			ds1 = 1;
			ds0 = 1;
			break;
		case SPPCTRL_DVIO_DRV_IOH_35300_IOL_43400UA:
			ds3 = 1;
			ds2 = 1;
			ds1 = 0;
			ds0 = 0;
			break;
		case SPPCTRL_DVIO_DRV_IOH_37800_IOL_46400UA:
			ds3 = 1;
			ds2 = 1;
			ds1 = 0;
			ds0 = 1;
			break;
		case SPPCTRL_DVIO_DRV_IOH_40300_IOL_49500UA:
			ds3 = 1;
			ds2 = 1;
			ds1 = 1;
			ds0 = 0;
			break;
		case SPPCTRL_DVIO_DRV_IOH_42700_IOL_52600UA:
			ds3 = 1;
			ds2 = 1;
			ds1 = 1;
			ds0 = 1;
			break;
		default:
			ret = -EINVAL;
			break;
		}
	} else {
		switch (value) {
		case SPPCTRL_GPIO_DRV_IOH_1100_IOL_1100UA:
			ds3 = 0;
			ds2 = 0;
			ds1 = 0;
			ds0 = 0;
			break;
		case SPPCTRL_GPIO_DRV_IOH_1600_IOL_1700UA:
			ds3 = 0;
			ds2 = 0;
			ds1 = 0;
			ds0 = 1;
			break;
		case SPPCTRL_GPIO_DRV_IOH_3300_IOL_3300UA:
			ds3 = 0;
			ds2 = 0;
			ds1 = 1;
			ds0 = 0;
			break;
		case SPPCTRL_GPIO_DRV_IOH_4900_IOL_5000UA:
			ds3 = 0;
			ds2 = 0;
			ds1 = 1;
			ds0 = 1;
			break;
		case SPPCTRL_GPIO_DRV_IOH_6600_IOL_6600UA:
			ds3 = 0;
			ds2 = 1;
			ds1 = 0;
			ds0 = 0;
			break;
		case SPPCTRL_GPIO_DRV_IOH_8200_IOL_8300UA:
			ds3 = 0;
			ds2 = 1;
			ds1 = 0;
			ds0 = 1;
			break;
		case SPPCTRL_GPIO_DRV_IOH_9900_IOL_9900UA:
			ds3 = 0;
			ds2 = 1;
			ds1 = 1;
			ds0 = 0;
			break;
		case SPPCTRL_GPIO_DRV_IOH_11500_IOL_11600UA:
			ds3 = 0;
			ds2 = 1;
			ds1 = 1;
			ds0 = 1;
			break;
		case SPPCTRL_GPIO_DRV_IOH_13100_IOL_13200UA:
			ds3 = 1;
			ds2 = 0;
			ds1 = 0;
			ds0 = 0;
			break;
		case SPPCTRL_GPIO_DRV_IOH_14800_IOL_14800UA:
			ds3 = 1;
			ds2 = 0;
			ds1 = 0;
			ds0 = 1;
			break;
		case SPPCTRL_GPIO_DRV_IOH_16400_IOL_16500UA:
			ds3 = 1;
			ds2 = 0;
			ds1 = 1;
			ds0 = 0;
			break;
		case SPPCTRL_GPIO_DRV_IOH_18100_IOL_18100UA:
			ds3 = 1;
			ds2 = 0;
			ds1 = 1;
			ds0 = 1;
			break;
		case SPPCTRL_GPIO_DRV_IOH_19600_IOL_19700UA:
			ds3 = 1;
			ds2 = 1;
			ds1 = 0;
			ds0 = 0;
			break;
		case SPPCTRL_GPIO_DRV_IOH_21300_IOL_21400UA:
			ds3 = 1;
			ds2 = 1;
			ds1 = 0;
			ds0 = 1;
			break;
		case SPPCTRL_GPIO_DRV_IOH_22900_IOL_23000UA:
			ds3 = 1;
			ds2 = 1;
			ds1 = 1;
			ds0 = 0;
			break;
		case SPPCTRL_GPIO_DRV_IOH_24600_IOL_24600UA:
			ds3 = 1;
			ds2 = 1;
			ds1 = 1;
			ds0 = 1;
			break;
		default:
			ret = -EINVAL;
			break;
		}
	}

	if (ret == 0) {
		r = readl(pc->padctl1_regs_base + REG_OFFSET_DS0 +
			  R32_ROF(selector));
		if (ds0 == 0)
			r &= ~BIT(R32_BOF(selector));
		else
			r |= BIT(R32_BOF(selector));

		writel(r, pc->padctl1_regs_base + REG_OFFSET_DS0 +
				  R32_ROF(selector));

		r = readl(pc->padctl1_regs_base + REG_OFFSET_DS1 +
			  R32_ROF(selector));

		if (ds1 == 0)
			r &= ~BIT(R32_BOF(selector));
		else
			r |= BIT(R32_BOF(selector));

		writel(r, pc->padctl1_regs_base + REG_OFFSET_DS1 +
				  R32_ROF(selector));

		r = readl(pc->padctl1_regs_base + REG_OFFSET_DS2 +
			  R32_ROF(selector));
		if (ds2 == 0)
			r &= ~BIT(R32_BOF(selector));
		else
			r |= BIT(R32_BOF(selector));

		writel(r, pc->padctl1_regs_base + REG_OFFSET_DS2 +
				  R32_ROF(selector));

		r = readl(pc->padctl1_regs_base + REG_OFFSET_DS3 +
			  R32_ROF(selector));
		if (ds3 == 0)
			r &= ~BIT(R32_BOF(selector));
		else
			r |= BIT(R32_BOF(selector));

		writel(r, pc->padctl1_regs_base + REG_OFFSET_DS3 +
				  R32_ROF(selector));
	}

	return ret;
}

int sppctl_gpio_drive_strength_get(struct gpio_chip *chip,
				   unsigned int selector)
{
	struct sppctlgpio_chip_t *pc;
	u32 ds0 = 0;
	u32 ds1 = 0;
	u32 ds2 = 0;
	u32 ds3 = 0;
	u32 r;

	pc = (struct sppctlgpio_chip_t *)gpiochip_get_data(chip);

	r = readl(pc->padctl1_regs_base + REG_OFFSET_DS0 + R32_ROF(selector));
	ds0 = R32_VAL(r, R32_BOF(selector));

	r = readl(pc->padctl1_regs_base + REG_OFFSET_DS1 + R32_ROF(selector));
	ds1 = R32_VAL(r, R32_BOF(selector));

	r = readl(pc->padctl1_regs_base + REG_OFFSET_DS2 + R32_ROF(selector));
	ds2 = R32_VAL(r, R32_BOF(selector));

	r = readl(pc->padctl1_regs_base + REG_OFFSET_DS3 + R32_ROF(selector));
	ds3 = R32_VAL(r, R32_BOF(selector));

	if (IS_DVIO(selector)) {
		if (ds3 == 0 && ds2 == 0 && ds1 == 0 && ds0 == 0)
			return SPPCTRL_DVIO_DRV_IOH_5100_IOL_6200UA;
		else if (ds3 == 0 && ds2 == 0 && ds1 == 0 && ds0 == 1)
			return SPPCTRL_DVIO_DRV_IOH_7600_IOL_9300UA;
		else if (ds3 == 0 && ds2 == 0 && ds1 == 1 && ds0 == 0)
			return SPPCTRL_DVIO_DRV_IOH_10100_IOL_12500UA;
		else if (ds3 == 0 && ds2 == 0 && ds1 == 1 && ds0 == 1)
			return SPPCTRL_DVIO_DRV_IOH_12600_IOL_15600UA;
		else if (ds3 == 0 && ds2 == 1 && ds1 == 0 && ds0 == 0)
			return SPPCTRL_DVIO_DRV_IOH_15200_IOL_18700UA;
		else if (ds3 == 0 && ds2 == 1 && ds1 == 0 && ds0 == 1)
			return SPPCTRL_DVIO_DRV_IOH_17700_IOL_21800UA;
		else if (ds3 == 0 && ds2 == 1 && ds1 == 1 && ds0 == 0)
			return SPPCTRL_DVIO_DRV_IOH_20200_IOL_24900UA;
		else if (ds3 == 0 && ds2 == 1 && ds1 == 1 && ds0 == 1)
			return SPPCTRL_DVIO_DRV_IOH_22700_IOL_27900UA;
		else if (ds3 == 1 && ds2 == 0 && ds1 == 0 && ds0 == 0)
			return SPPCTRL_DVIO_DRV_IOH_25200_IOL_31000UA;
		else if (ds3 == 1 && ds2 == 0 && ds1 == 0 && ds0 == 1)
			return SPPCTRL_DVIO_DRV_IOH_27700_IOL_34100UA;
		else if (ds3 == 1 && ds2 == 0 && ds1 == 1 && ds0 == 0)
			return SPPCTRL_DVIO_DRV_IOH_30300_IOL_37200UA;
		else if (ds3 == 1 && ds2 == 0 && ds1 == 1 && ds0 == 1)
			return SPPCTRL_DVIO_DRV_IOH_32800_IOL_40300UA;
		else if (ds3 == 1 && ds2 == 1 && ds1 == 0 && ds0 == 0)
			return SPPCTRL_DVIO_DRV_IOH_35300_IOL_43400UA;
		else if (ds3 == 1 && ds2 == 1 && ds1 == 0 && ds0 == 1)
			return SPPCTRL_DVIO_DRV_IOH_37800_IOL_46400UA;
		else if (ds3 == 1 && ds2 == 1 && ds1 == 1 && ds0 == 0)
			return SPPCTRL_DVIO_DRV_IOH_40300_IOL_49500UA;
		else if (ds3 == 1 && ds2 == 1 && ds1 == 1 && ds0 == 1)
			return SPPCTRL_DVIO_DRV_IOH_42700_IOL_52600UA;
		else
			return -EINVAL;
	} else {
		if (ds3 == 0 && ds2 == 0 && ds1 == 0 && ds0 == 0)
			return SPPCTRL_GPIO_DRV_IOH_1100_IOL_1100UA;
		else if (ds3 == 0 && ds2 == 0 && ds1 == 0 && ds0 == 1)
			return SPPCTRL_GPIO_DRV_IOH_1600_IOL_1700UA;
		else if (ds3 == 0 && ds2 == 0 && ds1 == 1 && ds0 == 0)
			return SPPCTRL_GPIO_DRV_IOH_3300_IOL_3300UA;
		else if (ds3 == 0 && ds2 == 0 && ds1 == 1 && ds0 == 1)
			return SPPCTRL_GPIO_DRV_IOH_4900_IOL_5000UA;
		else if (ds3 == 0 && ds2 == 1 && ds1 == 0 && ds0 == 0)
			return SPPCTRL_GPIO_DRV_IOH_6600_IOL_6600UA;
		else if (ds3 == 0 && ds2 == 1 && ds1 == 0 && ds0 == 1)
			return SPPCTRL_GPIO_DRV_IOH_8200_IOL_8300UA;
		else if (ds3 == 0 && ds2 == 1 && ds1 == 1 && ds0 == 0)
			return SPPCTRL_GPIO_DRV_IOH_9900_IOL_9900UA;
		else if (ds3 == 0 && ds2 == 1 && ds1 == 1 && ds0 == 1)
			return SPPCTRL_GPIO_DRV_IOH_11500_IOL_11600UA;
		else if (ds3 == 1 && ds2 == 0 && ds1 == 0 && ds0 == 0)
			return SPPCTRL_GPIO_DRV_IOH_13100_IOL_13200UA;
		else if (ds3 == 1 && ds2 == 0 && ds1 == 0 && ds0 == 1)
			return SPPCTRL_GPIO_DRV_IOH_14800_IOL_14800UA;
		else if (ds3 == 1 && ds2 == 0 && ds1 == 1 && ds0 == 0)
			return SPPCTRL_GPIO_DRV_IOH_16400_IOL_16500UA;
		else if (ds3 == 1 && ds2 == 0 && ds1 == 1 && ds0 == 1)
			return SPPCTRL_GPIO_DRV_IOH_18100_IOL_18100UA;
		else if (ds3 == 1 && ds2 == 1 && ds1 == 0 && ds0 == 0)
			return SPPCTRL_GPIO_DRV_IOH_19600_IOL_19700UA;
		else if (ds3 == 1 && ds2 == 1 && ds1 == 0 && ds0 == 1)
			return SPPCTRL_GPIO_DRV_IOH_21300_IOL_21400UA;
		else if (ds3 == 1 && ds2 == 1 && ds1 == 1 && ds0 == 0)
			return SPPCTRL_GPIO_DRV_IOH_22900_IOL_23000UA;
		else if (ds3 == 1 && ds2 == 1 && ds1 == 1 && ds0 == 1)
			return SPPCTRL_GPIO_DRV_IOH_24600_IOL_24600UA;
		else
			return -EINVAL;
	}
}

int sppctl_gpio_request(struct gpio_chip *chip, unsigned int selector)
{
	sppctl_gpio_first_master_set(chip, selector, MUX_FIRST_G, MUX_MASTER_G);

	return gpiochip_generic_request(chip, selector);
}

void sppctl_gpio_free(struct gpio_chip *chip, unsigned int selector)
{
	gpiochip_generic_free(chip, selector);
}

#if defined(SUPPORT_GPIO_AO_INT)
int find_gpio_ao_int(struct sppctlgpio_chip_t *pc, int pin)
{
	int i;

	for (i = 0; i < 32; i++)
		if (pin == pc->gpio_ao_int_pins[i])
			return i;
	return -1;
}
#endif

// get dir: 0=out, 1=in, -E =err (-EINVAL for ex): OE inverted on ret
int sppctl_gpio_get_direction(struct gpio_chip *chip, unsigned int selector)
{
	struct sppctlgpio_chip_t *pc;
#if defined(SUPPORT_GPIO_AO_INT)
	int ao_pin;
	int mask;
#endif
	u32 r;

	pc = (struct sppctlgpio_chip_t *)gpiochip_get_data(chip);

#if defined(SUPPORT_GPIO_AO_INT)
	ao_pin = find_gpio_ao_int(pc, selector);
	if (ao_pin >= 0) {
		mask = 1 << ao_pin;
		if (readl(pc->gpio_ao_int_regs_base + 0x18) & mask) // GPIO_OE
			return 0;
		else
			return 1;
	}
#endif
	r = readl(pc->gpioxt_regs_base + REG_OFFSET_OUTPUT_ENABLE +
		  R16_ROF(selector));
	return R32_VAL(r, R16_BOF(selector)) ^ BIT(0);
}

// set to input: 0:ok: OE=0
int sppctl_gpio_direction_input(struct gpio_chip *chip, unsigned int selector)
{
	struct sppctlgpio_chip_t *pc;
#if defined(SUPPORT_GPIO_AO_INT)
	int ao_pin;
	int mask;
#endif
	u32 r;

	pc = (struct sppctlgpio_chip_t *)gpiochip_get_data(chip);

#if defined(SUPPORT_GPIO_AO_INT)
	ao_pin = find_gpio_ao_int(pc, selector);
	if (ao_pin >= 0) {
		mask = 1 << ao_pin;
		r = readl(pc->gpio_ao_int_regs_base + 0x18); // GPIO_OE
		r &= ~mask;
		writel(r, pc->gpio_ao_int_regs_base + 0x18); // GPIO_OE
		return 0;
	}
#endif

	r = (BIT(R16_BOF(selector)) << 16);
	writel(r, pc->gpioxt_regs_base + REG_OFFSET_OUTPUT_ENABLE +
			  R16_ROF(selector));

	return 0;
}

/* input enable or disable */
int sppctl_gpio_input_enable_set(struct gpio_chip *chip, unsigned int selector,
				 int value)
{
	struct sppctlgpio_chip_t *pc;
#if defined(SUPPORT_GPIO_AO_INT)
	int ao_pin;
	int mask;
#endif
	u32 r;

	pc = (struct sppctlgpio_chip_t *)gpiochip_get_data(chip);

	if (value < 0)
		return -EINVAL;

#if defined(SUPPORT_GPIO_AO_INT)
	ao_pin = find_gpio_ao_int(pc, selector);
	if (ao_pin >= 0) {
		mask = 1 << ao_pin;

		r = readl(pc->gpio_ao_int_regs_base + 0x18); // GPIO_OE
		if (value == 1)
			r &= ~mask; /* enable */
		else
			r |= mask; /* disable */

		writel(r, pc->gpio_ao_int_regs_base + 0x18); // GPIO_OE
		return 0;
	}
#endif

	r = (BIT(R16_BOF(selector)) << 16) |
	    ((!value & BIT(0)) << R16_BOF(selector));
	writel(r, pc->gpioxt_regs_base + REG_OFFSET_OUTPUT_ENABLE +
			  R16_ROF(selector));

	return 0;
}

int sppctl_gpio_input_enable_query(struct gpio_chip *chip,
				   unsigned int selector)
{
	struct sppctlgpio_chip_t *pc;
#if defined(SUPPORT_GPIO_AO_INT)
	int ao_pin;
	int mask;
#endif
	u32 r;

	pc = (struct sppctlgpio_chip_t *)gpiochip_get_data(chip);

#if defined(SUPPORT_GPIO_AO_INT)
	ao_pin = find_gpio_ao_int(pc, selector);
	if (ao_pin >= 0) {
		mask = 1 << ao_pin;

		r = readl(pc->gpio_ao_int_regs_base + 0x18); // GPIO_OE
		if (r & mask)
			return 0;
		else
			return 1;
	}
#endif

	r = readl(pc->gpioxt_regs_base + REG_OFFSET_OUTPUT_ENABLE +
		  R16_ROF(selector));

	return !R32_VAL(r, R16_BOF(selector));
}

/* output enable or disable */
int sppctl_gpio_output_enable_set(struct gpio_chip *chip, unsigned int selector,
				  int value)
{
	struct sppctlgpio_chip_t *pc;
#if defined(SUPPORT_GPIO_AO_INT)
	int ao_pin;
	int mask;
#endif
	u32 r;

	pc = (struct sppctlgpio_chip_t *)gpiochip_get_data(chip);

	if (value < 0)
		return -EINVAL;

#if defined(SUPPORT_GPIO_AO_INT)
	ao_pin = find_gpio_ao_int(pc, selector);
	if (ao_pin >= 0) {
		mask = 1 << ao_pin;

		r = readl(pc->gpio_ao_int_regs_base + 0x18); // GPIO_OE
		if (value == 1)
			r |= mask; /* enable */
		else
			r &= ~mask; /* disable */

		writel(r, pc->gpio_ao_int_regs_base + 0x18); // GPIO_OE
		return 0;
	}
#endif

	r = (BIT(R16_BOF(selector)) << 16) |
	    ((value & BIT(0)) << R16_BOF(selector));
	writel(r, pc->gpioxt_regs_base + REG_OFFSET_OUTPUT_ENABLE +
			  R16_ROF(selector));

	return 0;
}

int sppctl_gpio_output_enable_query(struct gpio_chip *chip,
				    unsigned int selector)
{
	struct sppctlgpio_chip_t *pc;
#if defined(SUPPORT_GPIO_AO_INT)
	int ao_pin;
	int mask;
#endif
	u32 r;

	pc = (struct sppctlgpio_chip_t *)gpiochip_get_data(chip);

#if defined(SUPPORT_GPIO_AO_INT)
	ao_pin = find_gpio_ao_int(pc, selector);
	if (ao_pin >= 0) {
		mask = 1 << ao_pin;

		r = readl(pc->gpio_ao_int_regs_base + 0x18); // GPIO_OE
		if (r & mask)
			return 1; /* enable */
		else
			return 0; /* disable */
	}
#endif

	r = readl(pc->gpioxt_regs_base + REG_OFFSET_OUTPUT_ENABLE +
		  R16_ROF(selector));

	return R32_VAL(r, R16_BOF(selector));
}

// set to output: 0:ok: OE=1,O=value
int sppctl_gpio_direction_output(struct gpio_chip *chip, unsigned int selector,
				 int value)
{
	struct sppctlgpio_chip_t *pc;
#if defined(SUPPORT_GPIO_AO_INT)
	int ao_pin;
	int mask;
#endif
	u32 r;

	pc = (struct sppctlgpio_chip_t *)gpiochip_get_data(chip);

#if defined(SUPPORT_GPIO_AO_INT)
	ao_pin = find_gpio_ao_int(pc, selector);
	if (ao_pin >= 0) {
		mask = 1 << ao_pin;

		r = readl(pc->gpio_ao_int_regs_base + 0x18); // GPIO_OE
		r |= mask;
		writel(r, pc->gpio_ao_int_regs_base + 0x18); // GPIO_OE

		if (value < 0)
			return 0;

		r = readl(pc->gpio_ao_int_regs_base + 0x14); // GPIO_O
		if (value)
			r |= mask;
		else
			r &= ~mask;
		writel(r, pc->gpio_ao_int_regs_base + 0x14); // GPIO_O
		return 0;
	}
#endif
	r = (BIT(R16_BOF(selector)) << 16) | BIT(R16_BOF(selector));
	writel(r, pc->gpioxt_regs_base + REG_OFFSET_OUTPUT_ENABLE +
			  R16_ROF(selector));

	if (value < 0)
		return 0;

	r = (BIT(R16_BOF(selector)) << 16) |
	    ((value & BIT(0)) << R16_BOF(selector));

	writel(r, pc->gpioxt_regs_base + REG_OFFSET_OUTPUT + R16_ROF(selector));

	sppctl_gpio_unmux_irq(chip, selector);

	return 0;
}

// set to output: 0:ok: OE=1,O=value
int sppctl_gpio_direction_output_query(struct gpio_chip *chip,
				       unsigned int selector)
{
	struct sppctlgpio_chip_t *pc;
#if defined(SUPPORT_GPIO_AO_INT)
	int ao_pin;
	int mask;
#endif
	u32 r;

	pc = (struct sppctlgpio_chip_t *)gpiochip_get_data(chip);

#if defined(SUPPORT_GPIO_AO_INT)
	ao_pin = find_gpio_ao_int(pc, selector);
	if (ao_pin >= 0) {
		mask = 1 << ao_pin;

		r = readl(pc->gpio_ao_int_regs_base + 0x14); // GPIO_O
		if (r & mask)
			return 1;
		else
			return 0;
	}
#endif
	r = readl(pc->gpioxt_regs_base + REG_OFFSET_OUTPUT + R16_ROF(selector));

	return R32_VAL(r, R16_BOF(selector));
}

// get value for signal: 0=low | 1=high | -err
int sppctl_gpio_get_value(struct gpio_chip *chip, unsigned int selector)
{
	struct sppctlgpio_chip_t *pc;
#if defined(SUPPORT_GPIO_AO_INT)
	int ao_pin;
	int mask;
#endif
	u32 r;

	pc = (struct sppctlgpio_chip_t *)gpiochip_get_data(chip);

#if defined(SUPPORT_GPIO_AO_INT)
	ao_pin = find_gpio_ao_int(pc, selector);
	if (ao_pin >= 0) {
		mask = 1 << ao_pin;
		if (readl(pc->gpio_ao_int_regs_base + 0x08) &
		    mask) // GPIO_DEB_EN
			r = readl(pc->gpio_ao_int_regs_base +
				  0x10); // GPIO_DEB_I
		else
			r = readl(pc->gpio_ao_int_regs_base + 0x0c); // GPIO_I
		if (r & mask)
			return 1;
		else
			return 0;
	}
#endif
	r = readl(pc->gpioxt_regs_base + REG_OFFSET_INPUT + R32_ROF(selector));

	return R32_VAL(r, R32_BOF(selector));
}

// OUT only: can't call set on IN pin: protected by gpio_chip layer
void sppctl_gpio_set_value(struct gpio_chip *chip, unsigned int selector,
			   int value)
{
	struct sppctlgpio_chip_t *pc;
#if defined(SUPPORT_GPIO_AO_INT)
	int ao_pin;
	int mask;
#endif
	u32 r;

	pc = (struct sppctlgpio_chip_t *)gpiochip_get_data(chip);

#if defined(SUPPORT_GPIO_AO_INT)
	ao_pin = find_gpio_ao_int(pc, selector);
	if (ao_pin >= 0) {
		mask = 1 << ao_pin;
		r = readl(pc->gpio_ao_int_regs_base + 0x14); // GPIO_O
		if (value)
			r |= mask;
		else
			r &= ~mask;
		writel(r, pc->gpio_ao_int_regs_base + 0x14); // GPIO_O
		return;
	}
#endif
	r = (BIT(R16_BOF(selector)) << 16) | (value & BIT(0))
						     << R16_BOF(selector);
	writel(r, pc->gpioxt_regs_base + REG_OFFSET_OUTPUT + R16_ROF(selector));
}

// FIX: test in-depth
int sppctl_gpio_set_config(struct gpio_chip *chip, unsigned int selector,
			   unsigned long config)
{
	enum pin_config_param config_param;
	u32 config_arg;
	int ret = 0;

#if defined(SUPPORT_GPIO_AO_INT)
	struct sppctlgpio_chip_t *pc;
	int ao_pin;

	pc = (struct sppctlgpio_chip_t *)gpiochip_get_data(chip);
	ao_pin = find_gpio_ao_int(pc, selector);
#endif

	config_param = pinconf_to_config_param(config);
	config_arg = pinconf_to_config_argument(config);

	KDBG(chip->parent, "f_scf(%03d,%lX) p:%d a:%d\n", selector, config,
	     config_param, config_arg);
	switch (config_param) {
	case PIN_CONFIG_DRIVE_OPEN_DRAIN:
#if defined(SUPPORT_GPIO_AO_INT)
		if (ao_pin >= 0)
			return -ENOTSUPP;
#endif
		sppctl_gpio_open_drain_mode_set(chip, selector, 1);
		break;

	case PIN_CONFIG_INPUT_ENABLE:
		KERR(chip->parent, "f_scf(%03d,%lX) input enable arg:%d\n",
		     selector, config, config_arg);
		break;

	case PIN_CONFIG_OUTPUT:
		ret = sppctl_gpio_direction_output(chip, selector, config_arg);
		break;

	case PIN_CONFIG_PERSIST_STATE:
		KDBG(chip->parent, "f_scf(%03d,%lX) not support pinconf:%d\n",
		     selector, config, config_param);
		ret = -ENOTSUPP;
		break;

	case PIN_CONFIG_DRIVE_STRENGTH:
		sppctl_gpio_drive_strength_set(chip, selector,
					       config_arg * 1000);
		break;

	case PIN_CONFIG_DRIVE_STRENGTH_UA:
		sppctl_gpio_drive_strength_set(chip, selector, config_arg);
		break;

	case PIN_CONFIG_INPUT_SCHMITT_ENABLE:
		sppctl_gpio_schmitt_trigger_set(chip, selector, config_arg);
		break;

	default:
		KDBG(chip->parent, "f_scf(%03d,%lX) unknown pinconf:%d\n",
		     selector, config, config_param);
		ret = -EINVAL;
		break;
	}

	return ret;
}

#ifdef CONFIG_DEBUG_FS
void sppctl_gpio_dbg_show(struct seq_file *seq, struct gpio_chip *chip)
{
	const char *label;
	int i;

	for (i = 0; i < chip->ngpio; i++) {
		label = gpiochip_is_requested(chip, i);
		if (!label)
			label = "";

		seq_printf(seq, " gpio-%03d (%-16.16s | %-16.16s)",
			   i + chip->base, chip->names[i], label);
		seq_printf(seq, " %c",
			   sppctl_gpio_get_direction(chip, i) == 0 ? 'O' : 'I');
		seq_printf(seq, ":%d", sppctl_gpio_get_value(chip, i));
		seq_printf(seq, " %s",
			   (sppctl_gpio_first_get(chip, i) ? "gpio" : "mux"));
		seq_printf(seq, " %s",
			   (sppctl_gpio_is_inverted(chip, i) ? "inv" : "   "));
		seq_printf(seq, " %s",
			   (sppctl_gpio_open_drain_mode_query(chip, i) ? "oDr" :
									       ""));
		seq_puts(seq, "\n");
	}
}
#else
#define sppctl_gpio_dbg_show NULL
#endif

int sppctl_gpio_to_irq(struct gpio_chip *chip, unsigned int offset)
{
	return -ENXIO;
}

void sppctl_gpio_unmux_irq(struct gpio_chip *chip, unsigned int selector)
{
}
