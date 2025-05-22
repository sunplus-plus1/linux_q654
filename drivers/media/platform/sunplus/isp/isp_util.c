// SPDX-License-Identifier: GPL-2.0+
/*
 * Driver for ISP
 *
 */
#include "isp_util.h"

#define DEBUG_REG_BUFF_MESG (false)

void __iomem *isp_reg_base = NULL;
void __iomem *isp_reg_base_2 = NULL;
static int reg_base_en = 0;

void __iomem *isp_hw_base_addr = NULL;
static int hw_reg_buff_idx = 0;

void isp_video_set_reg_base(void __iomem *isp_base, void __iomem *isp_base_2, int en)
{
    isp_reg_base = isp_base;
    isp_reg_base_2 = isp_base_2;
    reg_base_en = en;
}

void __iomem *isp_video_next_reg_buff(void)
{
    if (reg_base_en == 0)
    {
        printk("%s: base addr was not set \n", __func__);
        return NULL;
    }

    if (ISP_REG_BUFF_NUM == 1) // shuttle case
    {
        isp_hw_base_addr = isp_reg_base;
        hw_reg_buff_idx = 0;
    }
    else if (ISP_REG_BUFF_NUM == 2)
    {
        // mp case, switch to next buffer
        switch (hw_reg_buff_idx)
        {
        case 0:
        {
            isp_hw_base_addr = isp_reg_base_2;
            hw_reg_buff_idx = 1;
        }
        break;
        case 1:
        {
            isp_hw_base_addr = isp_reg_base;
            hw_reg_buff_idx = 0;
        }
        break;
        default:
            printk("%s:wrong hw_reg_buff_idx\n", __func__);
            return NULL;
        }
    }
    return isp_hw_base_addr;
}

void __iomem *isp_video_get_reg_base(void)
{
    return isp_hw_base_addr;
}

int isp_video_get_reg_buff_idx(void)
{
    return hw_reg_buff_idx;
}

long isp_video_write_reg_from_buff(u8 *ker_rq_addr)
{
    struct FrameSetting *head_addr = NULL;
    struct Settings *reg_setting = NULL;
    u8 *cur_data = ker_rq_addr;
    size_t idx = 0;
    enum ISP_ID group_idx = 0;
    enum ISP_ID actual_idx = 0;
    size_t reg_num = 0;

    if (reg_base_en == 0)
    {
        printk("%s: base addr was not set \n", __func__);
        return -EAGAIN;
    }

    if (isp_hw_base_addr == NULL)
    {
        printk("%s: get hw based addr failed\n", __func__);
        return -EAGAIN;
    }

    if (ker_rq_addr == NULL)
    {
        printk("%s: wrong input based addr\n", __func__);
        return -EAGAIN;
    }

    for (group_idx = 0; group_idx < ISP_SETTINGS_END; group_idx++)
    {
        head_addr = (struct FrameSetting *)cur_data;
        reg_num = head_addr->num_settings;
        actual_idx = head_addr->id;

        if (actual_idx != group_idx)
        {
            printk("%s(%d) !!!write reg buff error!!! \n", __func__,
                   __LINE__);
            printk("%s(%d) id not match, ideal=%d  real=%d\n",
                   __func__, __LINE__, group_idx, actual_idx);

            if (DEBUG_REG_BUFF_MESG)
                isp_video_print_reg_buff(ker_rq_addr,
                                         ISP_REGQ_SIZE,
                                         ISP_SETTINGS_END);
            break;
        }
        else if (reg_num == 0)
        {
            printk("%s(%d) !!!write reg buff error!!! \n", __func__,
                   __LINE__);
            printk("%s(%d) reg num is zero (id=%d) \n", __func__,
                   __LINE__, actual_idx);

            if (DEBUG_REG_BUFF_MESG)
                isp_video_print_reg_buff(ker_rq_addr,
                                         ISP_REGQ_SIZE,
                                         ISP_SETTINGS_END);
            break;
        }
        else if (reg_num > 0)
        {
            cur_data = (u8 *)&(head_addr->settings[0]);

            for (idx = 0; idx < reg_num; idx++)
            {
                reg_setting = (struct Settings *)cur_data;
                if (reg_setting->dirty == 1)
                {
                    if (reg_setting->offset >
                        REG_MAX_OFFSET)
                    {
                        printk("%s(%d) reg offset out of range = 0x%x \n",
                               __func__, __LINE__,
                               reg_setting->offset);
                        continue;
                    }
                    writel(reg_setting->value,
                           isp_hw_base_addr +
                               reg_setting->offset);

                    if (DEBUG_REG_BUFF_MESG)
                    {
                        u32 real_val = readl(
                            isp_hw_base_addr +
                            reg_setting->offset);

                        if (real_val !=
                            reg_setting->value)
                            printk("%s(%d) isp reg diff !!! offset(0x%x) real = 0x%x  ideal = 0x%x  \n",
                                   __func__,
                                   __LINE__,
                                   reg_setting
                                       ->offset,
                                   real_val,
                                   reg_setting
                                       ->value);
                    }
                }
                cur_data += sizeof(struct Settings);
            }
        }
    }
    return 0;
}

void isp_video_print_reg_buff(u8 *usr_reg_addr, size_t buf_size,
                              size_t group_size)
{
    size_t idx = 0;
    struct FrameSetting *head_addr = NULL;
    size_t reg_num = 0;
    enum ISP_ID actual_idx = 0;
    size_t group_idx = 0;
    struct Settings *reg_setting = NULL;

    printk("====== isp driver : print reg buff start ======\n");

    printk("reg_buff addr: 0x%p \n", usr_reg_addr);
    printk("reg_buff size: 0x%lx \n", buf_size);

    buf_size = (buf_size + 9) / 10;

    for (idx = 0; idx < buf_size; idx++)
    {
        printk("%x %x %x %x %x %x %x %x %x %x\n",
               *(usr_reg_addr + idx * 10),
               *(usr_reg_addr + idx * 10 + 1),
               *(usr_reg_addr + idx * 10 + 2),
               *(usr_reg_addr + idx * 10 + 3),
               *(usr_reg_addr + idx * 10 + 4),
               *(usr_reg_addr + idx * 10 + 5),
               *(usr_reg_addr + idx * 10 + 6),
               *(usr_reg_addr + idx * 10 + 7),
               *(usr_reg_addr + idx * 10 + 8),
               *(usr_reg_addr + idx * 10 + 9));
    }
    printk("====== isp driver : print reg buff end ======\n");

    printk("====== isp driver : print reg buff group start ======\n");
    for (group_idx = 0; group_idx < group_size; group_idx++)
    {
        head_addr = (struct FrameSetting *)usr_reg_addr;
        reg_num = head_addr->num_settings;
        actual_idx = head_addr->id;
        printk("=== id(%x)  reg_num(0x%lx)\n", actual_idx, reg_num);

        usr_reg_addr = (u8 *)&(head_addr->settings[0]);

        for (idx = 0; idx < reg_num; idx++)
        {
            reg_setting = (struct Settings *)usr_reg_addr;

            printk("%x %x %x\n", reg_setting->dirty,
                   reg_setting->offset, reg_setting->value);

            usr_reg_addr += sizeof(struct Settings);
        }
    }
    printk("====== isp driver : print reg buff group end ======\n");
}
