// SPDX-License-Identifier: GPL-2.0+
/*
 * Driver for ISP
 *
 */
#ifndef ISP_REG_H
#define ISP_REG_H

#define ISP_START_LAUNCH                         0x0
#define ISP_CONTROL_0                            0x4
#define ISP_APB_REG_SEL                          0x8
#define ISP_FUNC_EN_0                            0xC
#define ISP_FUNC_EN_1                           0x10
#define ISP_FUNC_EN_2                           0x14
#define ISP_SW_RESET_EN                         0x1C
#define ISP_INTERRUPT_EN                        0x30
#define ISP_INTERRUPT_OUT                       0x34
#define ISP_INTERRUPT_CLR                       0x38
#define ISP_INTERRUPT_STS                       0x3C
#define ISP_PATGEN                              0x40
#define ISP_CR_REGION_VLD                       0x44
#define ISP_SNIFFER_CR_ADDR                     0x48
#define ISP_SNIFFER_CR_DATA                     0x4C
#define ISP_AXI_QOS_0                           0x50
#define ISP_AXI_QOS_1                           0x54
#define ISP_AXI_QOS_2                           0x58
#define ISP_AXI_QOS_3                           0x5C
#define ISP_DNR_INOUT_FORMAT_CTRL               0x64
#define ISP_IN_FORMAT_CTRL                      0x68
#define ISP_OUT_FORMAT_CTRL                     0x6C
#define ISP_DMA_0                               0x70
#define ISP_DMA_2                               0x74
#define ISP_DMA_4                               0x78
#define ISP_DMA_6                               0x7C
#define ISP_DMA_8                               0x80
#define ISP_DMA_10                              0x84
#define ISP_DMA_12                              0x88
#define ISP_DMA_14                              0x8C
#define ISP_DMA_16                              0x90
#define ISP_DMA_18                              0x94
#define ISP_DMA_20                              0x98
#define ISP_DMA_22                              0x9C
#define ISP_DMA_24                              0xA0
#define ISP_DMA_26                              0xA4
#define ISP_DMA_28                              0xA8
#define ISP_DMA_30                              0xAC
#define ISP_DMA_32                              0xB0
#define ISP_DMA_34                              0xB4
#define ISP_DMA_36                              0xB8
#define ISP_DMA_38                              0xBC
#define ISP_DMA_40                              0xC0
#define ISP_DMA_42                              0xC4
#define ISP_DMA_44                              0xC8
#define ISP_DMA_46                              0xCC
#define ISP_DMA_47                              0xD0
#define ISP_DMA_48                              0xD4
#define ISP_DMA_49                              0xD8
#define ISP_DMA_50                              0xDC
#define ISP_DMA_51                              0xE0
#define ISP_DMA_52                              0xE4
#define ISP_FCURVE_DMA_WH                       0xEC
#define ISP_MLSC_DMA_WH                         0xF0
#define ISP_WDR_DMA_WH                          0xF4
#define ISP_LCE_DMA_WH                          0xF8
#define ISP_CNR_DMA_WH                          0xFC
#define ISP_HDR_0                              0x100
#define ISP_HDR_1                              0x104
#define ISP_HDR_2                              0x108
#define ISP_HDR_3                              0x10C
#define ISP_HDR_4                              0x110
#define ISP_HDR_5                              0x114
#define ISP_HDR_6                              0x118
#define ISP_HDR_7                              0x11C
#define ISP_HDR_8                              0x120
#define ISP_HDR_9                              0x124
#define ISP_HDR_10                             0x128
#define ISP_HDR_11                             0x12C
#define ISP_HDR_12                             0x130
#define ISP_FCURVE_0                           0x150
#define ISP_FCURVE_1                           0x154
#define ISP_FCURVE_2                           0x158
#define ISP_FCURVE_3                           0x15C
#define ISP_FCURVE_4                           0x160
#define ISP_FCURVE_5                           0x164
#define ISP_FCURVE_6                           0x168
#define ISP_FCURVE_7                           0x16C
#define ISP_FCURVE_8                           0x170
#define ISP_FCURVE_9                           0x174
#define ISP_FCURVE_10                          0x178
#define ISP_FCURVE_11                          0x17C
#define ISP_FCURVE_12                          0x180
#define ISP_FCURVE_13                          0x188
#define ISP_FCURVE_14                          0x18C
#define ISP_FCURVE_15                          0x190
#define ISP_FCURVE_16                          0x194
#define ISP_FCURVE_17                          0x1A0
#define ISP_FCURVE_18                          0x1A4
#define ISP_FCURVE_19                          0x1A8
#define ISP_FCURVE_20                          0x1AC
#define ISP_FCURVE_21                          0x1B0
#define ISP_FCURVE_22                          0x1B4
#define ISP_FCURVE_23                          0x1B8
#define ISP_FCURVE_24                          0x1BC
#define ISP_FCURVE_25                          0x1C0
#define ISP_FCURVE_26                          0x1C4
#define ISP_FCURVE_27                          0x1C8
#define ISP_FCURVE_28                          0x1CC
#define ISP_FCURVE_29                          0x1D0
#define ISP_FCURVE_30                          0x1D4
#define ISP_FCURVE_31                          0x1D8
#define ISP_FCURVE_32                          0x1DC
#define ISP_FCURVE_33                          0x1E0
#define ISP_FCURVE_34                          0x1E4
#define ISP_FCURVE_35                          0x1E8
#define ISP_FCURVE_36                          0x1EC
#define ISP_FCURVE_37                          0x1F0
#define ISP_FCURVE_38                          0x1F4
#define ISP_FCURVE_39                          0x1F8
#define ISP_FCURVE_40                          0x1FC
#define ISP_FCURVE_41                          0x200
#define ISP_FCURVE_42                          0x204
#define ISP_FCURVE_43                          0x208
#define ISP_FCURVE_44                          0x20C
#define ISP_FCURVE_45                          0x210
#define ISP_FCURVE_46                          0x214
#define ISP_FCURVE_47                          0x218
#define ISP_FCURVE_48                          0x21C
#define ISP_FCURVE_49                          0x220
#define ISP_FCURVE_50                          0x224
#define ISP_FCURVE_51                          0x228
#define ISP_FCURVE_52                          0x22C
#define ISP_FCURVE_53                          0x230
#define ISP_FCURVE_54                          0x234
#define ISP_FCURVE_55                          0x238
#define ISP_FCURVE_56                          0x23C
#define ISP_FCURVE_57                          0x240
#define ISP_FCURVE_58                          0x244
#define ISP_FCURVE_59                          0x248
#define ISP_FCURVE_60                          0x24C
#define ISP_FCURVE_61                          0x250
#define ISP_FCURVE_62                          0x254
#define ISP_FCURVE_63                          0x258
#define ISP_FCURVE_64                          0x25C
#define ISP_FCURVE_65                          0x260
#define ISP_FCURVE_66                          0x264
#define ISP_FCURVE_67                          0x268
#define ISP_FCURVE_68                          0x26C
#define ISP_FCURVE_69                          0x270
#define ISP_FCURVE_70                          0x274
#define ISP_FCURVE_71                          0x278
#define ISP_FCURVE_72                          0x27C
#define ISP_FCURVE_73                          0x280
#define ISP_FCURVE_74                          0x284
#define ISP_FCURVE_75                          0x288
#define ISP_FCURVE_76                          0x28C
#define ISP_FCURVE_77                          0x290
#define ISP_FCURVE_78                          0x294
#define ISP_FCURVE_79                          0x298
#define ISP_FCURVE_80                          0x29C
#define ISP_FCURVE_81                          0x2A0
#define ISP_FCURVE_82                          0x2A4
#define ISP_FCURVE_83                          0x2A8
#define ISP_FCURVE_84                          0x2AC
#define ISP_FCURVE_85                          0x2B0
#define ISP_FCURVE_86                          0x2B4
#define ISP_FCURVE_87                          0x2B8
#define ISP_FCURVE_88                          0x2BC
#define ISP_FCURVE_89                          0x2C0
#define ISP_FCURVE_90                          0x2C4
#define ISP_FCURVE_91                          0x2C8
#define ISP_FCURVE_92                          0x2CC
#define ISP_FCURVE_93                          0x2D0
#define ISP_FCURVE_94                          0x2D4
#define ISP_FCURVE_95                          0x2D8
#define ISP_FCURVE_96                          0x2DC
#define ISP_FCURVE_97                          0x2E0
#define ISP_FCURVE_98                          0x2E4
#define ISP_FCURVE_99                          0x2E8
#define ISP_FCURVE_100                         0x2EC
#define ISP_FCURVE_101                         0x2F0
#define ISP_FCURVE_102                         0x2F4
#define ISP_FCURVE_103                         0x2F8
#define ISP_FCURVE_104                         0x2FC
#define ISP_FCURVE_105                         0x300
#define ISP_FCURVE_106                         0x304
#define ISP_FCURVE_107                         0x308
#define ISP_FCURVE_108                         0x30C
#define ISP_FCURVE_109                         0x310
#define ISP_FCURVE_110                         0x314
#define ISP_FCURVE_111                         0x318
#define ISP_FCURVE_112                         0x31C
#define ISP_FCURVE_113                         0x320
#define ISP_FCURVE_114                         0x324
#define ISP_FCURVE_115                         0x328
#define ISP_FCURVE_116                         0x32C
#define ISP_FCURVE_117                         0x330
#define ISP_FCURVE_118                         0x334
#define ISP_FCURVE_119                         0x338
#define ISP_FCURVE_120                         0x33C
#define ISP_FCURVE_121                         0x340
#define ISP_FCURVE_122                         0x344
#define ISP_FCURVE_123                         0x348
#define ISP_FCURVE_124                         0x34C
#define ISP_FCURVE_125                         0x350
#define ISP_FCURVE_126                         0x354
#define ISP_FCURVE_127                         0x358
#define ISP_FCURVE_128                         0x35C
#define ISP_FCURVE_129                         0x360
#define ISP_FCURVE_130                         0x364
#define ISP_FCURVE_131                         0x368
#define ISP_FCURVE_132                         0x36C
#define ISP_FCURVE_133                         0x370
#define ISP_FCURVE_134                         0x374
#define ISP_FCURVE_135                         0x378
#define ISP_FCURVE_136                         0x37C
#define ISP_FCURVE_137                         0x380
#define ISP_FCURVE_138                         0x384
#define ISP_FCURVE_139                         0x388
#define ISP_FCURVE_140                         0x38C
#define ISP_FCURVE_141                         0x390
#define ISP_FCURVE_142                         0x394
#define ISP_FCURVE_143                         0x398
#define ISP_FCURVE_144                         0x39C
#define ISP_FCURVE_145                         0x3A0
#define ISP_FCURVE_146                         0x3B0
#define ISP_FCURVE_147                         0x3B4
#define ISP_FCURVE_148                         0x3B8
#define ISP_FCURVE_149                         0x3BC
#define ISP_FCURVE_150                         0x3C0
#define ISP_FCURVE_151                         0x3C4
#define ISP_FCURVE_152                         0x3C8
#define ISP_FCURVE_153                         0x3CC
#define ISP_FCURVE_154                         0x3D0
#define ISP_FCURVE_155                         0x3D4
#define ISP_FCURVE_156                         0x3D8
#define ISP_FCURVE_157                         0x3DC
#define ISP_FCURVE_158                         0x3E0
#define ISP_FCURVE_159                         0x3E4
#define ISP_FCURVE_160                         0x3E8
#define ISP_FCURVE_161                         0x3EC
#define ISP_FCURVE_162                         0x3F0
#define ISP_FCURVE_163                         0x3F4
#define ISP_FCURVE_164                         0x3F8
#define ISP_FCURVE_165                         0x3FC
#define ISP_FCURVE_166                         0x400
#define ISP_FCURVE_167                         0x404
#define ISP_FCURVE_168                         0x408
#define ISP_FCURVE_169                         0x40C
#define ISP_FCURVE_170                         0x410
#define ISP_FCURVE_171                         0x414
#define ISP_FCURVE_172                         0x418
#define ISP_FCURVE_173                         0x41C
#define ISP_FCURVE_174                         0x420
#define ISP_FCURVE_175                         0x424
#define ISP_FCURVE_176                         0x428
#define ISP_FCURVE_177                         0x42C
#define ISP_FCURVE_178                         0x430
#define ISP_FCURVE_179                         0x434
#define ISP_FCURVE_180                         0x438
#define ISP_FCURVE_181                         0x43C
#define ISP_FCURVE_182                         0x440
#define ISP_FCURVE_183                         0x444
#define ISP_FCURVE_184                         0x448
#define ISP_FCURVE_185                         0x44C
#define ISP_FCURVE_186                         0x450
#define ISP_FCURVE_187                         0x454
#define ISP_FCURVE_188                         0x458
#define ISP_FCURVE_189                         0x45C
#define ISP_FCURVE_190                         0x460
#define ISP_FCURVE_191                         0x464
#define ISP_FCURVE_192                         0x468
#define ISP_FCURVE_193                         0x46C
#define ISP_FCURVE_194                         0x470
#define ISP_FCURVE_195                         0x474
#define ISP_FCURVE_196                         0x478
#define ISP_FCURVE_197                         0x47C
#define ISP_FCURVE_198                         0x480
#define ISP_FCURVE_199                         0x484
#define ISP_FCURVE_200                         0x488
#define ISP_FCURVE_201                         0x48C
#define ISP_FCURVE_202                         0x490
#define ISP_FCURVE_203                         0x494
#define ISP_FCURVE_204                         0x498
#define ISP_FCURVE_205                         0x49C
#define ISP_FCURVE_206                         0x4A0
#define ISP_FCURVE_207                         0x4A4
#define ISP_FCURVE_208                         0x4A8
#define ISP_FCURVE_209                         0x4AC
#define ISP_FCURVE_210                         0x4B0
#define ISP_FCURVE_211                         0x4B4
#define ISP_FCURVE_212                         0x4B8
#define ISP_FCURVE_213                         0x4BC
#define ISP_FCURVE_214                         0x4C0
#define ISP_FCURVE_215                         0x4C4
#define ISP_FCURVE_216                         0x4C8
#define ISP_FCURVE_217                         0x4CC
#define ISP_FCURVE_218                         0x4D0
#define ISP_FCURVE_219                         0x4D4
#define ISP_FCURVE_220                         0x4D8
#define ISP_FCURVE_221                         0x4DC
#define ISP_FCURVE_222                         0x4E0
#define ISP_FCURVE_223                         0x4E4
#define ISP_FCURVE_224                         0x4E8
#define ISP_FCURVE_225                         0x4EC
#define ISP_FCURVE_226                         0x4F0
#define ISP_FCURVE_227                         0x4F4
#define ISP_FCURVE_228                         0x4F8
#define ISP_FCURVE_229                         0x4FC
#define ISP_FCURVE_230                         0x500
#define ISP_FCURVE_231                         0x504
#define ISP_FCURVE_232                         0x508
#define ISP_FCURVE_233                         0x50C
#define ISP_FCURVE_234                         0x510
#define ISP_FCURVE_235                         0x514
#define ISP_FCURVE_236                         0x518
#define ISP_FCURVE_237                         0x51C
#define ISP_FCURVE_238                         0x520
#define ISP_FCURVE_239                         0x524
#define ISP_FCURVE_240                         0x528
#define ISP_FCURVE_241                         0x52C
#define ISP_FCURVE_242                         0x530
#define ISP_FCURVE_243                         0x534
#define ISP_FCURVE_244                         0x538
#define ISP_FCURVE_245                         0x53C
#define ISP_FCURVE_246                         0x540
#define ISP_FCURVE_247                         0x544
#define ISP_FCURVE_248                         0x548
#define ISP_FCURVE_249                         0x54C
#define ISP_FCURVE_250                         0x550
#define ISP_FCURVE_251                         0x554
#define ISP_FCURVE_252                         0x558
#define ISP_FCURVE_253                         0x55C
#define ISP_FCURVE_254                         0x560
#define ISP_FCURVE_255                         0x564
#define ISP_FCURVE_256                         0x568
#define ISP_FCURVE_257                         0x56C
#define ISP_FCURVE_258                         0x570
#define ISP_FCURVE_259                         0x574
#define ISP_FCURVE_260                         0x578
#define ISP_FCURVE_261                         0x57C
#define ISP_FCURVE_262                         0x580
#define ISP_FCURVE_263                         0x584
#define ISP_FCURVE_264                         0x588
#define ISP_FCURVE_265                         0x58C
#define ISP_FCURVE_266                         0x590
#define ISP_FCURVE_267                         0x594
#define ISP_FCURVE_268                         0x598
#define ISP_FCURVE_269                         0x59C
#define ISP_FCURVE_270                         0x5A0
#define ISP_FCURVE_271                         0x5A4
#define ISP_FCURVE_272                         0x5A8
#define ISP_FCURVE_273                         0x5AC
#define ISP_FCURVE_274                         0x5B0
#define ISP_RGBIR_0                            0x5C0
#define ISP_RGBIR_1                            0x5C8
#define ISP_RGBIR_2                            0x5CC
#define ISP_RGBIR_3                            0x5D0
#define ISP_RGBIR_4                            0x5D4
#define ISP_RGBIR_5                            0x5D8
#define ISP_RGBIR_6                            0x5DC
#define ISP_RGBIR_7                            0x5E0
#define ISP_RGBIR_8                            0x5E4
#define ISP_RGBIR_9                            0x5E8
#define ISP_RGBIR_10                           0x5EC
#define ISP_RGBIR_11                           0x5F0
#define ISP_DPC_0                              0x600
#define ISP_DPC_1                              0x604
#define ISP_DPC_2                              0x608
#define ISP_DPC_3                              0x60C
#define ISP_DPC_4                              0x610
#define ISP_DPC_5                              0x614
#define ISP_DPC_6                              0x618
#define ISP_GE_0                               0x630
#define ISP_GE_1                               0x634
#define ISP_MLSC_0                             0x650
#define ISP_MLSC_1                             0x654
#define ISP_MLSC_2                             0x658
#define ISP_RLSC_0                             0x660
#define ISP_RLSC_1                             0x664
#define ISP_RLSC_2                             0x670
#define ISP_RLSC_3                             0x674
#define ISP_RLSC_4                             0x678
#define ISP_RLSC_5                             0x67C
#define ISP_RLSC_6                             0x680
#define ISP_RLSC_7                             0x684
#define ISP_RLSC_8                             0x688
#define ISP_RLSC_9                             0x68C
#define ISP_RLSC_10                            0x690
#define ISP_RLSC_11                            0x694
#define ISP_RLSC_12                            0x698
#define ISP_RLSC_13                            0x69C
#define ISP_RLSC_14                            0x6A0
#define ISP_RLSC_15                            0x6A4
#define ISP_RLSC_16                            0x6A8
#define ISP_RLSC_17                            0x6AC
#define ISP_RLSC_18                            0x6B0
#define ISP_RLSC_19                            0x6C0
#define ISP_RLSC_20                            0x6C4
#define ISP_RLSC_21                            0x6C8
#define ISP_RLSC_22                            0x6CC
#define ISP_RLSC_23                            0x6D0
#define ISP_RLSC_24                            0x6D4
#define ISP_RLSC_25                            0x6D8
#define ISP_RLSC_26                            0x6DC
#define ISP_RLSC_27                            0x6E0
#define ISP_RLSC_28                            0x6E4
#define ISP_RLSC_29                            0x6E8
#define ISP_RLSC_30                            0x6EC
#define ISP_RLSC_31                            0x6F0
#define ISP_RLSC_32                            0x6F4
#define ISP_RLSC_33                            0x6F8
#define ISP_RLSC_34                            0x6FC
#define ISP_RLSC_35                            0x700
#define ISP_RLSC_36                            0x710
#define ISP_RLSC_37                            0x714
#define ISP_RLSC_38                            0x718
#define ISP_RLSC_39                            0x71C
#define ISP_RLSC_40                            0x720
#define ISP_RLSC_41                            0x724
#define ISP_RLSC_42                            0x728
#define ISP_RLSC_43                            0x72C
#define ISP_RLSC_44                            0x730
#define ISP_RLSC_45                            0x734
#define ISP_RLSC_46                            0x738
#define ISP_RLSC_47                            0x73C
#define ISP_RLSC_48                            0x740
#define ISP_RLSC_49                            0x744
#define ISP_RLSC_50                            0x748
#define ISP_RLSC_51                            0x74C
#define ISP_RLSC_52                            0x750
#define ISP_BNR_0                              0x770
#define ISP_BNR_1                              0x774
#define ISP_BNR_2                              0x778
#define ISP_BNR_3                              0x77C
#define ISP_BNR_4                              0x780
#define ISP_BNR_5                              0x784
#define ISP_BNR_6                              0x788
#define ISP_BNR_7                              0x78C
#define ISP_BNR_8                              0x790
#define ISP_BNR_9                              0x794
#define ISP_BNR_10                             0x798
#define ISP_BNR_11                             0x79C
#define ISP_BNR_12                             0x7A0
#define ISP_BNR_13                             0x7A4
#define ISP_BNR_14                             0x7A8
#define ISP_BNR_15                             0x7AC
#define ISP_BNR_16                             0x7B0
#define ISP_BNR_17                             0x7B4
#define ISP_BNR_18                             0x7B8
#define ISP_BNR_19                             0x7BC
#define ISP_BNR_20                             0x7C0
#define ISP_WB_0                               0x7E0
#define ISP_WB_1                               0x7E4
#define ISP_WB_2                               0x7E8
#define ISP_WB_3                               0x7EC
#define ISP_WDR_0                              0x800
#define ISP_WDR_1                              0x804
#define ISP_WDR_2                              0x808
#define ISP_WDR_3                              0x80C
#define ISP_WDR_4                              0x810
#define ISP_WDR_5                              0x814
#define ISP_WDR_6                              0x818
#define ISP_WDR_7                              0x81C
#define ISP_WDR_8                              0x820
#define ISP_WDR_9                              0x824
#define ISP_WDR_10                             0x828
#define ISP_WDR_11                             0x82C
#define ISP_WDR_12                             0x830
#define ISP_WDR_13                             0x840
#define ISP_WDR_14                             0x844
#define ISP_WDR_15                             0x848
#define ISP_WDR_16                             0x84C
#define ISP_WDR_17                             0x850
#define ISP_WDR_18                             0x860
#define ISP_WDR_19                             0x864
#define ISP_WDR_20                             0x868
#define ISP_WDR_21                             0x86C
#define ISP_WDR_22                             0x870
#define ISP_WDR_23                             0x880
#define ISP_WDR_24                             0x884
#define ISP_WDR_25                             0x888
#define ISP_WDR_26                             0x88C
#define ISP_WDR_27                             0x890
#define ISP_WDR_28                             0x894
#define ISP_WDR_29                             0x8B0
#define ISP_WDR_30                             0x8B4
#define ISP_WDR_31                             0x8B8
#define ISP_WDR_32                             0x8BC
#define ISP_WDR_33                             0x8C0
#define ISP_WDR_34                             0x8C4
#define ISP_WDR_35                             0x8C8
#define ISP_WDR_36                             0x8CC
#define ISP_WDR_37                             0x8D0
#define ISP_WDR_38                             0x8D4
#define ISP_WDR_39                             0x8D8
#define ISP_WDR_40                             0x8DC
#define ISP_WDR_41                             0x8E0
#define ISP_WDR_42                             0x8E4
#define ISP_WDR_43                             0x8E8
#define ISP_WDR_44                             0x8EC
#define ISP_WDR_45                             0x8F0
#define ISP_WDR_46                             0x8F4
#define ISP_WDR_47                             0x8F8
#define ISP_WDR_48                             0x8FC
#define ISP_WDR_49                             0x900
#define ISP_WDR_50                             0x904
#define ISP_WDR_51                             0x908
#define ISP_WDR_52                             0x90C
#define ISP_WDR_53                             0x910
#define ISP_WDR_54                             0x914
#define ISP_WDR_55                             0x918
#define ISP_WDR_56                             0x91C
#define ISP_WDR_57                             0x920
#define ISP_WDR_58                             0x924
#define ISP_WDR_59                             0x928
#define ISP_WDR_60                             0x92C
#define ISP_WDR_61                             0x930
#define ISP_WDR_62                             0x934
#define ISP_WDR_63                             0x938
#define ISP_WDR_64                             0x93C
#define ISP_WDR_65                             0x940
#define ISP_WDR_66                             0x944
#define ISP_WDR_67                             0x948
#define ISP_WDR_68                             0x94C
#define ISP_WDR_69                             0x950
#define ISP_WDR_70                             0x954
#define ISP_WDR_71                             0x958
#define ISP_WDR_72                             0x95C
#define ISP_WDR_73                             0x960
#define ISP_WDR_74                             0x964
#define ISP_WDR_75                             0x968
#define ISP_WDR_76                             0x96C
#define ISP_WDR_77                             0x970
#define ISP_WDR_78                             0x974
#define ISP_WDR_79                             0x978
#define ISP_WDR_80                             0x97C
#define ISP_WDR_81                             0x980
#define ISP_WDR_82                             0x984
#define ISP_WDR_83                             0x988
#define ISP_WDR_84                             0x98C
#define ISP_WDR_85                             0x990
#define ISP_WDR_86                             0x994
#define ISP_WDR_87                             0x998
#define ISP_WDR_88                             0x99C
#define ISP_WDR_89                             0x9A0
#define ISP_WDR_90                             0x9A4
#define ISP_WDR_91                             0x9A8
#define ISP_WDR_92                             0x9AC
#define ISP_WDR_93                             0x9B0
#define ISP_DM_0                               0xA00
#define ISP_DM_1                               0xA04
#define ISP_DM_2                               0xA08
#define ISP_DM_3                               0xA0C
#define ISP_DM_4                               0xA10
#define ISP_DM_5                               0xA14
#define ISP_DM_6                               0xA18
#define ISP_DM_7                               0xA20
#define ISP_DM_8                               0xA24
#define ISP_DM_9                               0xA28
#define ISP_DM_10                              0xA2C
#define ISP_DM_11                              0xA30
#define ISP_DM_12                              0xA34
#define ISP_DM_13                              0xA40
#define ISP_DM_14                              0xA44
#define ISP_DM_15                              0xA48
#define ISP_DM_16                              0xA4C
#define ISP_DM_17                              0xA50
#define ISP_DM_18                              0xA54
#define ISP_DM_19                              0xA58
#define ISP_DM_20                              0xA5C
#define ISP_DM_21                              0xA60
#define ISP_DM_22                              0xA64
#define ISP_DM_23                              0xA68
#define ISP_DM_24                              0xA6C
#define ISP_DM_25                              0xA70
#define ISP_DM_26                              0xA74
#define ISP_DM_27                              0xA78
#define ISP_DM_28                              0xA7C
#define ISP_DM_29                              0xA80
#define ISP_DM_30                              0xA84
#define ISP_CCM_0                              0xAA0
#define ISP_CCM_1                              0xAA4
#define ISP_CCM_2                              0xAA8
#define ISP_CCM_3                              0xAAC
#define ISP_CCM_4                              0xAB0
#define ISP_CCM_5                              0xAC0
#define ISP_CCM_6                              0xAC4
#define ISP_CCM_7                              0xAC8
#define ISP_CCM_8                              0xACC
#define ISP_CCM_9                              0xAD0
#define ISP_CCM_10                             0xAD4
#define ISP_CCM_11                             0xAD8
#define ISP_CCM_12                             0xADC
#define ISP_CCM_13                             0xAE0
#define ISP_CCM_14                             0xAE4
#define ISP_GAMMA_0                            0xB00
#define ISP_GAMMA_1                            0xB04
#define ISP_GAMMA_2                            0xB08
#define ISP_GAMMA_3                            0xB0C
#define ISP_GAMMA_4                            0xB10
#define ISP_GAMMA_5                            0xB14
#define ISP_GAMMA_6                            0xB18
#define ISP_GAMMA_7                            0xB1C
#define ISP_GAMMA_8                            0xB20
#define ISP_GAMMA_9                            0xB24
#define ISP_GAMMA_10                           0xB28
#define ISP_GAMMA_11                           0xB2C
#define ISP_GAMMA_12                           0xB30
#define ISP_GAMMA_13                           0xB34
#define ISP_GAMMA_14                           0xB38
#define ISP_GAMMA_15                           0xB3C
#define ISP_GAMMA_16                           0xB40
#define ISP_GAMMA_17                           0xB44
#define ISP_GAMMA_18                           0xB48
#define ISP_GAMMA_19                           0xB4C
#define ISP_GAMMA_20                           0xB50
#define ISP_GAMMA_21                           0xB54
#define ISP_GAMMA_22                           0xB58
#define ISP_GAMMA_23                           0xB5C
#define ISP_GAMMA_24                           0xB60
#define ISP_GAMMA_25                           0xB64
#define ISP_GAMMA_26                           0xB68
#define ISP_GAMMA_27                           0xB6C
#define ISP_GAMMA_28                           0xB70
#define ISP_GAMMA_29                           0xB74
#define ISP_GAMMA_30                           0xB78
#define ISP_GAMMA_31                           0xB7C
#define ISP_GAMMA_32                           0xB80
#define ISP_GAMMA_33                           0xB84
#define ISP_GAMMA_34                           0xB88
#define ISP_GAMMA_35                           0xB8C
#define ISP_GAMMA_36                           0xB90
#define ISP_GAMMA_37                           0xB94
#define ISP_GAMMA_38                           0xB98
#define ISP_GAMMA_39                           0xB9C
#define ISP_GAMMA_40                           0xBA0
#define ISP_GAMMA_41                           0xBA4
#define ISP_GAMMA_42                           0xBA8
#define ISP_CST_0                              0xBC0
#define ISP_CST_1                              0xBC4
#define ISP_CST_2                              0xBC8
#define ISP_CST_3                              0xBCC
#define ISP_CST_4                              0xBD0
#define ISP_CST_5                              0xBD4
#define ISP_CST_6                              0xBD8
#define ISP_CST_7                              0xBDC
#define ISP_CST_8                              0xBE0
#define ISP_CST_9                              0xBE4
#define ISP_CST_10                             0xBE8
#define ISP_CST_11                             0xBEC
#define ISP_LCE_0                              0xC00
#define ISP_LCE_1                              0xC04
#define ISP_LCE_2                              0xC08
#define ISP_LCE_3                              0xC0C
#define ISP_LCE_4                              0xC10
#define ISP_LCE_5                              0xC14
#define ISP_LCE_6                              0xC18
#define ISP_LCE_7                              0xC1C
#define ISP_LCE_8                              0xC20
#define ISP_LCE_9                              0xC24
#define ISP_LCE_10                             0xC28
#define ISP_CNR_0                              0xC40
#define ISP_CNR_1                              0xC44
#define ISP_CNR_2                              0xC48
#define ISP_CNR_3                              0xC4C
#define ISP_CNR_4                              0xC50
#define ISP_CNR_5                              0xC54
#define ISP_CNR_6                              0xC58
#define ISP_CNR_7                              0xC5C
#define ISP_CNR_8                              0xC60
#define ISP_CNR_9                              0xC64
#define ISP_CNR_10                             0xC68
#define ISP_CNR_11                             0xC6C
#define ISP_CNR_12                             0xC70
#define ISP_YCURVE_0                           0xC7C
#define ISP_YCURVE_1                           0xC80
#define ISP_YCURVE_2                           0xC84
#define ISP_YCURVE_3                           0xC88
#define ISP_YCURVE_4                           0xC8C
#define ISP_YCURVE_5                           0xC90
#define ISP_YCURVE_6                           0xC94
#define ISP_YCURVE_7                           0xC98
#define ISP_YCURVE_8                           0xC9C
#define ISP_YCURVE_9                           0xCA0
#define ISP_YCURVE_10                          0xCA4
#define ISP_YCURVE_11                          0xCA8
#define ISP_YCURVE_12                          0xCAC
#define ISP_YCURVE_13                          0xCB0
#define ISP_YCURVE_14                          0xCB4
#define ISP_YCURVE_15                          0xCB8
#define ISP_YCURVE_16                          0xCBC
#define ISP_YCURVE_17                          0xCC0
#define ISP_YCURVE_18                          0xCC4
#define ISP_YCURVE_19                          0xCC8
#define ISP_YCURVE_20                          0xCCC
#define ISP_YCURVE_21                          0xCD0
#define ISP_YCURVE_22                          0xCD4
#define ISP_YCURVE_23                          0xCD8
#define ISP_YCURVE_24                          0xCDC
#define ISP_YCURVE_25                          0xCE0
#define ISP_YCURVE_26                          0xCE4
#define ISP_YCURVE_27                          0xCE8
#define ISP_YCURVE_28                          0xCEC
#define ISP_YCURVE_29                          0xCF0
#define ISP_YCURVE_30                          0xCF4
#define ISP_YCURVE_31                          0xCF8
#define ISP_YCURVE_32                          0xCFC
#define ISP_SP_0                               0xD00
#define ISP_SP_1                               0xD04
#define ISP_SP_2                               0xD08
#define ISP_SP_3                               0xD0C
#define ISP_SP_4                               0xD10
#define ISP_SP_5                               0xD14
#define ISP_SP_6                               0xD18
#define ISP_SP_7                               0xD1C
#define ISP_SP_8                               0xD20
#define ISP_SP_9                               0xD24
#define ISP_SP_10                              0xD28
#define ISP_SP_11                              0xD2C
#define ISP_SP_12                              0xD30
#define ISP_SP_13                              0xD34
#define ISP_SP_14                              0xD38
#define ISP_SP_15                              0xD3C
#define ISP_SP_16                              0xD40
#define ISP_SP_17                              0xD44
#define ISP_SP_18                              0xD48
#define ISP_SP_19                              0xD4C
#define ISP_SP_20                              0xD50
#define ISP_SP_21                              0xD54
#define ISP_SP_22                              0xD58
#define ISP_SP_23                              0xD5C
#define ISP_SP_24                              0xD60
#define ISP_SP_25                              0xD64
#define ISP_SP_26                              0xD68
#define ISP_SP_27                              0xD6C
#define ISP_SP_28                              0xD70
#define ISP_SP_29                              0xD74
#define ISP_SP_30                              0xD78
#define ISP_SP_31                              0xD7C
#define ISP_SP_32                              0xD80
#define ISP_SP_33                              0xD84
#define ISP_SP_34                              0xD88
#define ISP_SP_35                              0xD8C
#define ISP_SP_36                              0xD90
#define ISP_NR3D_0                             0xE00
#define ISP_NR3D_1                             0xE04
#define ISP_NR3D_2                             0xE08
#define ISP_NR3D_3                             0xE0C
#define ISP_NR3D_4                             0xE10
#define ISP_NR3D_5                             0xE14
#define ISP_NR3D_6                             0xE18
#define ISP_NR3D_7                             0xE1C
#define ISP_NR3D_8                             0xE20
#define ISP_NR3D_9                             0xE24
#define ISP_NR3D_10                            0xE28
#define ISP_NR3D_11                            0xE2C
#define ISP_NR3D_12                            0xE30
#define ISP_NR3D_13                            0xE34
#define ISP_NR3D_14                            0xE38
#define ISP_NR3D_15                            0xE3C
#define ISP_NR3D_17                            0xE44
#define ISP_NR3D_18                            0xE48
#define ISP_NR3D_19                            0xE4C
#define ISP_NR3D_20                            0xE50
#define ISP_NR3D_21                            0xE54
#define ISP_NR3D_22                            0xE58
#define ISP_NR3D_23                            0xE5C
#define ISP_NR3D_24                            0xE60
#define ISP_NR3D_25                            0xE64
#define ISP_NR3D_26                            0xE68
#define ISP_NR3D_27                            0xE6C
#define ISP_NR3D_28                            0xE70
#define ISP_NR3D_29                            0xE74
#define ISP_SCALER_0                           0xF00
#define ISP_SCALER_1                           0xF04
#define ISP_SCALER_2                           0xF08
#define ISP_SCALER_3                           0xF0C
#define ISP_SCALER_4                           0xF10
#define ISP_SCALER_5                           0xF14
#define ISP_SCALER_6                           0xF18
#define ISP_SCALER_7                           0xF1C
#define ISP_SCALER_8                           0xF20
#define ISP_SCALER_9                           0xF24
#define ISP_SCALER_10                          0xF28
#define ISP_AWB_0                              0xF70
#define ISP_AWB_1                              0xF74
#define ISP_AWB_2                              0xF78
#define ISP_AWB_3                              0xF7C
#define ISP_AWB_4                              0xF80
#define ISP_AWB_5                              0xF84
#define ISP_AWB_6                              0xF88
#define ISP_AWB_7                              0xF8C
#define ISP_AE_0                               0xFA0
#define ISP_AE_1                               0xFA4
#define ISP_AE_2                               0xFA8
#define ISP_AE_3                               0xFAC
#define ISP_AE_4                               0xFB0
#define ISP_AE_5                               0xFB4
#define ISP_AE_6                               0xFB8
#define ISP_AE_7                               0xFBC
#define ISP_ISP_MONITOR0                      0x1000
#define ISP_ISP_MONITOR1                      0x1004
#define ISP_ISP_MONITOR2                      0x1008
#define ISP_ISP_MONITOR3                      0x100C
#define ISP_ISP_MONITOR4                      0x1010
#define ISP_ISP_MONITOR5                      0x1014
#define ISP_ISP_MONITOR6                      0x1018
#define ISP_ISP_MONITOR7                      0x101C
#define ISP_ISP_MONITOR8                      0x1020
#define ISP_ISP_MONITOR9                      0x1024
#define ISP_ISP_MONITOR10                     0x1028
#define ISP_ISP_MONITOR11                     0x102C
#define ISP_ISP_MONITOR12                     0x1030
#define ISP_ISP_MONITOR13                     0x1034
#define ISP_ISP_MONITOR14                     0x1038
#define ISP_ISP_MONITOR15                     0x103C
#define ISP_ISP_MONITOR16                     0x1040
#define ISP_ISP_MONITOR17                     0x1044
#define ISP_ISP_MONITOR18                     0x1048
#define ISP_ISP_MONITOR19                     0x104C
#define ISP_ISP_MONITOR20                     0x1050
#define ISP_ISP_MONITOR21                     0x1054
#define ISP_ISP_MONITOR22                     0x1058
#define ISP_ISP_MONITOR23                     0x105C
#define ISP_ISP_MONITOR24                     0x1060
#define ISP_ISP_MONITOR25                     0x1064
#define ISP_ISP_MONITOR26                     0x1068
#define ISP_ISP_MONITOR27                     0x106C
#define ISP_DMA_ADR_EXT0                      0x1070
#define ISP_DMA_ADR_EXT1                      0x1074
#define ISP_DMA_ADR_EXT2                      0x1078
#define ISP_DMA_ADR_EXT3                      0x107C
#define ISP_DMA_ADR_EXT4                      0x1080
#define ISP_DMA_ADR_EXT5                      0x1084
#define ISP_DMA_ADR_EXT6                      0x1088
#define ISP_ISP_SW0                           0x10A0
#define ISP_ISP_SW1                           0x10A4
#define ISP_ISP_SW2                           0x10A8
#define ISP_ISP_SW3                           0x10AC
#define ISP_ISP_SW4                           0x10B0
#define ISP_ISP_SW5                           0x10B4

/* ISP_START_LAUNCH */
#define START_LAUNCH_FRAME_START_MASK                                       0x1
#define START_LAUNCH_FRAME_START_LSB                                          0

/* ISP_CONTROL_0 */
#define CONTROL_0_WIDTH_MASK                                             0x1fff
#define CONTROL_0_WIDTH_LSB                                                   0
#define CONTROL_0_HEIGHT_MASK                                        0x1fff0000
#define CONTROL_0_HEIGHT_LSB                                                 16
#define CONTROL_0_BAYER_FORMAT_MASK                                  0x60000000
#define CONTROL_0_BAYER_FORMAT_LSB                                           29

/* ISP_APB_REG_SEL */
#define APB_REG_SEL_APB_REG_SEL_MASK                                        0x1
#define APB_REG_SEL_APB_REG_SEL_LSB                                           0

/* ISP_FUNC_EN_0 */
#define FUNC_EN_0_RGBIR_EN_MASK                                           0x100
#define FUNC_EN_0_RGBIR_EN_LSB                                                8
#define FUNC_EN_0_RGBIR_OUTIR_EN_MASK                                     0x200
#define FUNC_EN_0_RGBIR_OUTIR_EN_LSB                                          9
#define FUNC_EN_0_HDR_SENSOR_NUM_MASK                                    0x1000
#define FUNC_EN_0_HDR_SENSOR_NUM_LSB                                         12
#define FUNC_EN_0_HDR_FUSION_EN_MASK                                     0x2000
#define FUNC_EN_0_HDR_FUSION_EN_LSB                                          13
#define FUNC_EN_0_HDR_FUSION_OFF_SEL_MASK                                0x4000
#define FUNC_EN_0_HDR_FUSION_OFF_SEL_LSB                                     14
#define FUNC_EN_0_FCURVE_EN_MASK                                        0x10000
#define FUNC_EN_0_FCURVE_EN_LSB                                              16
#define FUNC_EN_0_FCURVE_SUBOUT_EN_MASK                                 0x20000
#define FUNC_EN_0_FCURVE_SUBOUT_EN_LSB                                       17
#define FUNC_EN_0_FCURVE_LOCAL_EN_MASK                                  0x40000
#define FUNC_EN_0_FCURVE_LOCAL_EN_LSB                                        18
#define FUNC_EN_0_DPC_EN_MASK                                          0x100000
#define FUNC_EN_0_DPC_EN_LSB                                                 20
#define FUNC_EN_0_GE_EN_MASK                                           0x200000
#define FUNC_EN_0_GE_EN_LSB                                                  21
#define FUNC_EN_0_MLSC_EN_MASK                                        0x1000000
#define FUNC_EN_0_MLSC_EN_LSB                                                24
#define FUNC_EN_0_RLSC_EN_MASK                                        0x2000000
#define FUNC_EN_0_RLSC_EN_LSB                                                25
#define FUNC_EN_0_BNR_EN_MASK                                        0x10000000
#define FUNC_EN_0_BNR_EN_LSB                                                 28
#define FUNC_EN_0_BNR_CENMOD_EN_MASK                                 0x20000000
#define FUNC_EN_0_BNR_CENMOD_EN_LSB                                          29

/* ISP_FUNC_EN_1 */
#define FUNC_EN_1_WDR_EN_MASK                                               0x1
#define FUNC_EN_1_WDR_EN_LSB                                                  0
#define FUNC_EN_1_WDR_SUBOUT_EN_MASK                                        0x2
#define FUNC_EN_1_WDR_SUBOUT_EN_LSB                                           1
#define FUNC_EN_1_WDR_DIFF_CONTROL_EN_MASK                                  0x4
#define FUNC_EN_1_WDR_DIFF_CONTROL_EN_LSB                                     2
#define FUNC_EN_1_WDR_OUTBLD_EN_MASK                                        0x8
#define FUNC_EN_1_WDR_OUTBLD_EN_LSB                                           3
#define FUNC_EN_1_DM_EN_MASK                                               0x10
#define FUNC_EN_1_DM_EN_LSB                                                   4
#define FUNC_EN_1_DM_HF_EN_MASK                                            0x20
#define FUNC_EN_1_DM_HF_EN_LSB                                                5
#define FUNC_EN_1_DM_SP_EN_MASK                                            0x40
#define FUNC_EN_1_DM_SP_EN_LSB                                                6
#define FUNC_EN_1_CCM_ENABLE_MASK                                         0x100
#define FUNC_EN_1_CCM_ENABLE_LSB                                              8
#define FUNC_EN_1_GAMMA_ENABLE_MASK                                       0x200
#define FUNC_EN_1_GAMMA_ENABLE_LSB                                            9
#define FUNC_EN_1_CST_ENABLE_MASK                                         0x400
#define FUNC_EN_1_CST_ENABLE_LSB                                             10
#define FUNC_EN_1_LCE_ENABLE_MASK                                        0x1000
#define FUNC_EN_1_LCE_ENABLE_LSB                                             12
#define FUNC_EN_1_LCE_SUBOUT_ENABLE_MASK                                 0x2000
#define FUNC_EN_1_LCE_SUBOUT_ENABLE_LSB                                      13
#define FUNC_EN_1_CNR_ENABLE_MASK                                       0x10000
#define FUNC_EN_1_CNR_ENABLE_LSB                                             16
#define FUNC_EN_1_CNR_SUBOUT_ENABLE_MASK                                0x20000
#define FUNC_EN_1_CNR_SUBOUT_ENABLE_LSB                                      17
#define FUNC_EN_1_YCURVE_ENABLE_MASK                                   0x100000
#define FUNC_EN_1_YCURVE_ENABLE_LSB                                          20
#define FUNC_EN_1_SP_ENABLE_MASK                                      0x1000000
#define FUNC_EN_1_SP_ENABLE_LSB                                              24
#define FUNC_EN_1_3DNR_ENABLE_MASK                                   0x10000000
#define FUNC_EN_1_3DNR_ENABLE_LSB                                            28
#define FUNC_EN_1_3DNR_DEBUG_ENABLE_MASK                             0x20000000
#define FUNC_EN_1_3DNR_DEBUG_ENABLE_LSB                                      29
#define FUNC_EN_1_3DNR_DEBUG_SEL_MASK                                0x40000000
#define FUNC_EN_1_3DNR_DEBUG_SEL_LSB                                         30
#define FUNC_EN_1_3DNR_FIRST_FRAME_MASK                              0x80000000
#define FUNC_EN_1_3DNR_FIRST_FRAME_LSB                                       31

/* ISP_FUNC_EN_2 */
#define FUNC_EN_2_SCALER_VUPDOWN_SEL_MASK                                   0x1
#define FUNC_EN_2_SCALER_VUPDOWN_SEL_LSB                                      0
#define FUNC_EN_2_HSCALE_EN_MASK                                            0x2
#define FUNC_EN_2_HSCALE_EN_LSB                                               1
#define FUNC_EN_2_VSCALE_EN_MASK                                            0x4
#define FUNC_EN_2_VSCALE_EN_LSB                                               2
#define FUNC_EN_2_HLPF_EN_MASK                                              0x8
#define FUNC_EN_2_HLPF_EN_LSB                                                 3
#define FUNC_EN_2_VLPF_EN_MASK                                             0x10
#define FUNC_EN_2_VLPF_EN_LSB                                                 4
#define FUNC_EN_2_SHARP_EN_MASK                                            0x20
#define FUNC_EN_2_SHARP_EN_LSB                                                5
#define FUNC_EN_2_AE_AWB_RGBIR_FLAG_MASK                                0x10000
#define FUNC_EN_2_AE_AWB_RGBIR_FLAG_LSB                                      16
#define FUNC_EN_2_AE_EN_MASK                                            0x20000
#define FUNC_EN_2_AE_EN_LSB                                                  17
#define FUNC_EN_2_AE_HIST_EN_MASK                                       0x40000
#define FUNC_EN_2_AE_HIST_EN_LSB                                             18
#define FUNC_EN_2_AE_EN2_MASK                                           0x80000
#define FUNC_EN_2_AE_EN2_LSB                                                 19
#define FUNC_EN_2_AE_HIST_EN2_MASK                                     0x100000
#define FUNC_EN_2_AE_HIST_EN2_LSB                                            20
#define FUNC_EN_2_AE_PATH_SEL_MASK                                     0x600000
#define FUNC_EN_2_AE_PATH_SEL_LSB                                            21
#define FUNC_EN_2_AWB_EN_MASK                                          0x800000
#define FUNC_EN_2_AWB_EN_LSB                                                 23
#define FUNC_EN_2_AWB_PATH_SEL_MASK                                   0x3000000
#define FUNC_EN_2_AWB_PATH_SEL_LSB                                           24

/* ISP_SW_RESET_EN */
#define SW_RESET_EN_ISP_CORE_RESET_EN_MASK                                  0x1
#define SW_RESET_EN_ISP_CORE_RESET_EN_LSB                                     0
#define SW_RESET_EN_ISP_IN_DMA_RESET_EN_MASK                                0x2
#define SW_RESET_EN_ISP_IN_DMA_RESET_EN_LSB                                   1
#define SW_RESET_EN_ISP_HDR_IN_DMA_RESET_EN_MASK                            0x4
#define SW_RESET_EN_ISP_HDR_IN_DMA_RESET_EN_LSB                               2
#define SW_RESET_EN_ISP_FCURVE_IN_DMA_RESET_EN_MASK                         0x8
#define SW_RESET_EN_ISP_FCURVE_IN_DMA_RESET_EN_LSB                            3
#define SW_RESET_EN_ISP_FCURVE_OUT_DMA_RESET_EN_MASK                       0x10
#define SW_RESET_EN_ISP_FCURVE_OUT_DMA_RESET_EN_LSB                           4
#define SW_RESET_EN_ISP_MLSC_IN_DMA_RESET_EN_MASK                          0x20
#define SW_RESET_EN_ISP_MLSC_IN_DMA_RESET_EN_LSB                              5
#define SW_RESET_EN_ISP_WDR_IN_DMA_RESET_EN_MASK                           0x40
#define SW_RESET_EN_ISP_WDR_IN_DMA_RESET_EN_LSB                               6
#define SW_RESET_EN_ISP_WDR_OUT_DMA_RESET_EN_MASK                          0x80
#define SW_RESET_EN_ISP_WDR_OUT_DMA_RESET_EN_LSB                              7
#define SW_RESET_EN_ISP_LCE_IN_DMA_RESET_EN_MASK                          0x100
#define SW_RESET_EN_ISP_LCE_IN_DMA_RESET_EN_LSB                               8
#define SW_RESET_EN_ISP_LCE_OUT_DMA_RESET_EN_MASK                         0x200
#define SW_RESET_EN_ISP_LCE_OUT_DMA_RESET_EN_LSB                              9
#define SW_RESET_EN_ISP_CNR_IN_DMA_RESET_EN_MASK                          0x400
#define SW_RESET_EN_ISP_CNR_IN_DMA_RESET_EN_LSB                              10
#define SW_RESET_EN_ISP_CNR_OUT_DMA_RESET_EN_MASK                         0x800
#define SW_RESET_EN_ISP_CNR_OUT_DMA_RESET_EN_LSB                             11
#define SW_RESET_EN_ISP_NR3D_Y_IN_DMA_RESET_EN_MASK                      0x1000
#define SW_RESET_EN_ISP_NR3D_Y_IN_DMA_RESET_EN_LSB                           12
#define SW_RESET_EN_ISP_NR3D_Y_OUT_DMA_RESET_EN_MASK                     0x2000
#define SW_RESET_EN_ISP_NR3D_Y_OUT_DMA_RESET_EN_LSB                          13
#define SW_RESET_EN_ISP_NR3D_UV_IN_DMA_RESET_EN_MASK                     0x4000
#define SW_RESET_EN_ISP_NR3D_UV_IN_DMA_RESET_EN_LSB                          14
#define SW_RESET_EN_ISP_NR3D_UV_OUT_DMA_RESET_EN_MASK                    0x8000
#define SW_RESET_EN_ISP_NR3D_UV_OUT_DMA_RESET_EN_LSB                         15
#define SW_RESET_EN_ISP_NR3D_MOTION_IN_DMA_RESET_EN_MASK                0x10000
#define SW_RESET_EN_ISP_NR3D_MOTION_IN_DMA_RESET_EN_LSB                      16
#define SW_RESET_EN_ISP_NR3D_MOTION_OUT_DMA_RESET_EN_MASK               0x20000
#define SW_RESET_EN_ISP_NR3D_MOTION_OUT_DMA_RESET_EN_LSB                     17
#define SW_RESET_EN_ISP_SCALER_Y_OUT_DMA_RESET_EN_MASK                  0x40000
#define SW_RESET_EN_ISP_SCALER_Y_OUT_DMA_RESET_EN_LSB                        18
#define SW_RESET_EN_ISP_SCALER_UV_OUT_DMA_RESET_EN_MASK                 0x80000
#define SW_RESET_EN_ISP_SCALER_UV_OUT_DMA_RESET_EN_LSB                       19
#define SW_RESET_EN_ISP_SCALER_U_OUT_DMA_RESET_EN_MASK                 0x100000
#define SW_RESET_EN_ISP_SCALER_U_OUT_DMA_RESET_EN_LSB                        20
#define SW_RESET_EN_ISP_SCALER_V_OUT_DMA_RESET_EN_MASK                 0x200000
#define SW_RESET_EN_ISP_SCALER_V_OUT_DMA_RESET_EN_LSB                        21
#define SW_RESET_EN_ISP_NR3D_U_IN_DMA_RESET_EN_MASK                    0x400000
#define SW_RESET_EN_ISP_NR3D_U_IN_DMA_RESET_EN_LSB                           22
#define SW_RESET_EN_ISP_NR3D_V_IN_DMA_RESET_EN_MASK                    0x800000
#define SW_RESET_EN_ISP_NR3D_V_IN_DMA_RESET_EN_LSB                           23
#define SW_RESET_EN_ISP_NR3D_U_OUT_DMA_RESET_EN_MASK                  0x1000000
#define SW_RESET_EN_ISP_NR3D_U_OUT_DMA_RESET_EN_LSB                          24
#define SW_RESET_EN_ISP_NR3D_V_OUT_DMA_RESET_EN_MASK                  0x2000000
#define SW_RESET_EN_ISP_NR3D_V_OUT_DMA_RESET_EN_LSB                          25

/* ISP_INTERRUPT_EN */
#define INTERRUPT_EN_LAST_PIXEL2VDMA_DONE_SEL_MASK                          0x1
#define INTERRUPT_EN_LAST_PIXEL2VDMA_DONE_SEL_LSB                             0

/* ISP_INTERRUPT_OUT */
#define INTERRUPT_OUT_HW_IRQ_MASK                                           0x1
#define INTERRUPT_OUT_HW_IRQ_LSB                                              0

/* ISP_INTERRUPT_CLR */
#define INTERRUPT_CLR_LAST_PIXEL2VDMA_DONE_CLR_MASK                         0x1
#define INTERRUPT_CLR_LAST_PIXEL2VDMA_DONE_CLR_LSB                            0

/* ISP_INTERRUPT_STS */
#define INTERRUPT_STS_LAST_PIXEL2VDMA_DONE_MASK                             0x1
#define INTERRUPT_STS_LAST_PIXEL2VDMA_DONE_LSB                                0

/* ISP_PATGEN */
#define PATGEN_EN_MASK                                                      0x1
#define PATGEN_EN_LSB                                                         0
#define PATGEN_MODE_MASK                                                    0xe
#define PATGEN_MODE_LSB                                                       1

/* ISP_CR_REGION_VLD */
#define CR_REGION_VLD_FUNC_GLOBAL_VLD_EN_MASK                               0x1
#define CR_REGION_VLD_FUNC_GLOBAL_VLD_EN_LSB                                  0
#define CR_REGION_VLD_DMA_CR_VLD_EN_MASK                                    0x2
#define CR_REGION_VLD_DMA_CR_VLD_EN_LSB                                       1
#define CR_REGION_VLD_HDR_CR_VLD_EN_MASK                                    0x4
#define CR_REGION_VLD_HDR_CR_VLD_EN_LSB                                       2
#define CR_REGION_VLD_FCURVE_CR_VLD_EN_MASK                                 0x8
#define CR_REGION_VLD_FCURVE_CR_VLD_EN_LSB                                    3
#define CR_REGION_VLD_DPC_CR_VLD_EN_MASK                                   0x10
#define CR_REGION_VLD_DPC_CR_VLD_EN_LSB                                       4
#define CR_REGION_VLD_GE_CR_VLD_EN_MASK                                    0x20
#define CR_REGION_VLD_GE_CR_VLD_EN_LSB                                        5
#define CR_REGION_VLD_LSC_CR_VLD_EN_MASK                                   0x40
#define CR_REGION_VLD_LSC_CR_VLD_EN_LSB                                       6
#define CR_REGION_VLD_BNR_CR_VLD_EN_MASK                                   0x80
#define CR_REGION_VLD_BNR_CR_VLD_EN_LSB                                       7
#define CR_REGION_VLD_WB_CR_VLD_EN_MASK                                   0x100
#define CR_REGION_VLD_WB_CR_VLD_EN_LSB                                        8
#define CR_REGION_VLD_WDR_CR_VLD_EN_MASK                                  0x200
#define CR_REGION_VLD_WDR_CR_VLD_EN_LSB                                       9
#define CR_REGION_VLD_DM_CR_VLD_EN_MASK                                   0x400
#define CR_REGION_VLD_DM_CR_VLD_EN_LSB                                       10
#define CR_REGION_VLD_CCM_CR_VLD_EN_MASK                                  0x800
#define CR_REGION_VLD_CCM_CR_VLD_EN_LSB                                      11
#define CR_REGION_VLD_GAMMA_CR_VLD_EN_MASK                               0x1000
#define CR_REGION_VLD_GAMMA_CR_VLD_EN_LSB                                    12
#define CR_REGION_VLD_CST_CR_VLD_EN_MASK                                 0x2000
#define CR_REGION_VLD_CST_CR_VLD_EN_LSB                                      13
#define CR_REGION_VLD_LCE_CR_VLD_EN_MASK                                 0x4000
#define CR_REGION_VLD_LCE_CR_VLD_EN_LSB                                      14
#define CR_REGION_VLD_CNR_CR_VLD_EN_MASK                                 0x8000
#define CR_REGION_VLD_CNR_CR_VLD_EN_LSB                                      15
#define CR_REGION_VLD_YCURVE_CR_VLD_EN_MASK                             0x10000
#define CR_REGION_VLD_YCURVE_CR_VLD_EN_LSB                                   16
#define CR_REGION_VLD_SP_CR_VLD_EN_MASK                                 0x20000
#define CR_REGION_VLD_SP_CR_VLD_EN_LSB                                       17
#define CR_REGION_VLD_NR3D_CR_VLD_EN_MASK                               0x40000
#define CR_REGION_VLD_NR3D_CR_VLD_EN_LSB                                     18
#define CR_REGION_VLD_SCALER_CR_VLD_EN_MASK                             0x80000
#define CR_REGION_VLD_SCALER_CR_VLD_EN_LSB                                   19
#define CR_REGION_VLD_AWB_CR_VLD_EN_MASK                               0x100000
#define CR_REGION_VLD_AWB_CR_VLD_EN_LSB                                      20
#define CR_REGION_VLD_AE_CR_VLD_EN_MASK                                0x200000
#define CR_REGION_VLD_AE_CR_VLD_EN_LSB                                       21
#define CR_REGION_VLD_RGBIR_CR_VLD_EN_MASK                             0x800000
#define CR_REGION_VLD_RGBIR_CR_VLD_EN_LSB                                    23

/* ISP_SNIFFER_CR_ADDR */
#define SNIFFER_CR_ADDR_TARGET_ADDR_MASK                             0xffffffff
#define SNIFFER_CR_ADDR_TARGET_ADDR_LSB                                       0

/* ISP_SNIFFER_CR_DATA */
#define SNIFFER_CR_DATA_TARGET_DATA_MASK                             0xffffffff
#define SNIFFER_CR_DATA_TARGET_DATA_LSB                                       0

/* ISP_AXI_QOS_0 */
#define AXI_QOS_0_LCE_W_DMA_MQOS_MASK                                       0xf
#define AXI_QOS_0_LCE_W_DMA_MQOS_LSB                                          0
#define AXI_QOS_0_LCE_R_DMA_MQOS_MASK                                      0xf0
#define AXI_QOS_0_LCE_R_DMA_MQOS_LSB                                          4
#define AXI_QOS_0_WDR_W_DMA_MQOS_MASK                                     0xf00
#define AXI_QOS_0_WDR_W_DMA_MQOS_LSB                                          8
#define AXI_QOS_0_WDR_R_DMA_MQOS_MASK                                    0xf000
#define AXI_QOS_0_WDR_R_DMA_MQOS_LSB                                         12
#define AXI_QOS_0_FCRV_W_DMA_MQOS_MASK                                  0xf0000
#define AXI_QOS_0_FCRV_W_DMA_MQOS_LSB                                        16
#define AXI_QOS_0_FCRV_R_DMA_MQOS_MASK                                 0xf00000
#define AXI_QOS_0_FCRV_R_DMA_MQOS_LSB                                        20
#define AXI_QOS_0_HDRIN_R_DMA_MQOS_MASK                               0xf000000
#define AXI_QOS_0_HDRIN_R_DMA_MQOS_LSB                                       24
#define AXI_QOS_0_IN_R_DMA_MQOS_MASK                                 0xf0000000
#define AXI_QOS_0_IN_R_DMA_MQOS_LSB                                          28

/* ISP_AXI_QOS_1 */
#define AXI_QOS_1_DNR_MOTION_W_DMA_MQOS_MASK                                0xf
#define AXI_QOS_1_DNR_MOTION_W_DMA_MQOS_LSB                                   0
#define AXI_QOS_1_DNR_MOTION_R_DMA_MQOS_MASK                               0xf0
#define AXI_QOS_1_DNR_MOTION_R_DMA_MQOS_LSB                                   4
#define AXI_QOS_1_DNR_UV_W_DMA_MQOS_MASK                                  0xf00
#define AXI_QOS_1_DNR_UV_W_DMA_MQOS_LSB                                       8
#define AXI_QOS_1_DNR_UV_R_DMA_MQOS_MASK                                 0xf000
#define AXI_QOS_1_DNR_UV_R_DMA_MQOS_LSB                                      12
#define AXI_QOS_1_DNR_Y_W_DMA_MQOS_MASK                                 0xf0000
#define AXI_QOS_1_DNR_Y_W_DMA_MQOS_LSB                                       16
#define AXI_QOS_1_DNR_Y_R_DMA_MQOS_MASK                                0xf00000
#define AXI_QOS_1_DNR_Y_R_DMA_MQOS_LSB                                       20
#define AXI_QOS_1_CNR_W_DMA_MQOS_MASK                                 0xf000000
#define AXI_QOS_1_CNR_W_DMA_MQOS_LSB                                         24
#define AXI_QOS_1_CNR_R_DMA_MQOS_MASK                                0xf0000000
#define AXI_QOS_1_CNR_R_DMA_MQOS_LSB                                         28

/* ISP_AXI_QOS_2 */
#define AXI_QOS_2_AE2_BA_W_DMA_MQOS_MASK                                    0xf
#define AXI_QOS_2_AE2_BA_W_DMA_MQOS_LSB                                       0
#define AXI_QOS_2_AE2_HIST_W_DMA_MQOS_MASK                                 0xf0
#define AXI_QOS_2_AE2_HIST_W_DMA_MQOS_LSB                                     4
#define AXI_QOS_2_AE_BA_W_DMA_MQOS_MASK                                   0xf00
#define AXI_QOS_2_AE_BA_W_DMA_MQOS_LSB                                        8
#define AXI_QOS_2_AE_HIST_W_DMA_MQOS_MASK                                0xf000
#define AXI_QOS_2_AE_HIST_W_DMA_MQOS_LSB                                     12
#define AXI_QOS_2_AWB_W_DMA_MQOS_MASK                                   0xf0000
#define AXI_QOS_2_AWB_W_DMA_MQOS_LSB                                         16
#define AXI_QOS_2_MLSC_R_DMA_MQOS_MASK                                 0xf00000
#define AXI_QOS_2_MLSC_R_DMA_MQOS_LSB                                        20
#define AXI_QOS_2_SCALER_Y_W_DMA_MQOS_MASK                            0xf000000
#define AXI_QOS_2_SCALER_Y_W_DMA_MQOS_LSB                                    24
#define AXI_QOS_2_SCALER_UV_W_DMA_MQOS_MASK                          0xf0000000
#define AXI_QOS_2_SCALER_UV_W_DMA_MQOS_LSB                                   28

/* ISP_AXI_QOS_3 */
#define AXI_QOS_3_SCALER_U_W_DMA_MQOS_MASK                                  0xf
#define AXI_QOS_3_SCALER_U_W_DMA_MQOS_LSB                                     0
#define AXI_QOS_3_SCALER_V_W_DMA_MQOS_MASK                                 0xf0
#define AXI_QOS_3_SCALER_V_W_DMA_MQOS_LSB                                     4
#define AXI_QOS_3_DNR_U_R_DMA_MQOS_MASK                                   0xf00
#define AXI_QOS_3_DNR_U_R_DMA_MQOS_LSB                                        8
#define AXI_QOS_3_DNR_V_R_DMA_MQOS_MASK                                  0xf000
#define AXI_QOS_3_DNR_V_R_DMA_MQOS_LSB                                       12
#define AXI_QOS_3_DNR_U_W_DMA_MQOS_MASK                                 0xf0000
#define AXI_QOS_3_DNR_U_W_DMA_MQOS_LSB                                       16
#define AXI_QOS_3_DNR_V_W_DMA_MQOS_MASK                                0xf00000
#define AXI_QOS_3_DNR_V_W_DMA_MQOS_LSB                                       20

/* ISP_DNR_INOUT_FORMAT_CTRL */
#define DNR_INOUT_FORMAT_CTRL_NR3D_YUV_FORMAT_IN_MASK                       0x7
#define DNR_INOUT_FORMAT_CTRL_NR3D_YUV_FORMAT_IN_LSB                          0
#define DNR_INOUT_FORMAT_CTRL_NR3D_YUV_FORMAT_OUT_MASK                     0x70
#define DNR_INOUT_FORMAT_CTRL_NR3D_YUV_FORMAT_OUT_LSB                         4
#define DNR_INOUT_FORMAT_CTRL_NR3D_YUV_REF_OUT_EN_MASK                  0x10000
#define DNR_INOUT_FORMAT_CTRL_NR3D_YUV_REF_OUT_EN_LSB                        16
#define DNR_INOUT_FORMAT_CTRL_NR3D_MOTION_REF_OUT_EN_MASK               0x20000
#define DNR_INOUT_FORMAT_CTRL_NR3D_MOTION_REF_OUT_EN_LSB                     17

/* ISP_IN_FORMAT_CTRL */
#define IN_FORMAT_CTRL_IN_RAW_WIDTH_SEL_MASK                                0x3
#define IN_FORMAT_CTRL_IN_RAW_WIDTH_SEL_LSB                                   0

/* ISP_OUT_FORMAT_CTRL */
#define OUT_FORMAT_CTRL_YUV_FORMAT_SEL_MASK                                 0x7
#define OUT_FORMAT_CTRL_YUV_FORMAT_SEL_LSB                                    0

/* ISP_DMA_0 */
#define DMA_0_DMA_ADDR_HDR_IN0_MASK                                  0xffffffff
#define DMA_0_DMA_ADDR_HDR_IN0_LSB                                            0

/* ISP_DMA_2 */
#define DMA_2_DMA_ADDR_HDR_IN1_MASK                                  0xffffffff
#define DMA_2_DMA_ADDR_HDR_IN1_LSB                                            0

/* ISP_DMA_4 */
#define DMA_4_DMA_ADDR_FCURVE_SUBIN_MASK                             0xffffffff
#define DMA_4_DMA_ADDR_FCURVE_SUBIN_LSB                                       0

/* ISP_DMA_6 */
#define DMA_6_DMA_ADDR_FCURVE_SUBOUT_MASK                            0xffffffff
#define DMA_6_DMA_ADDR_FCURVE_SUBOUT_LSB                                      0

/* ISP_DMA_8 */
#define DMA_8_DMA_ADDR_MLSC_MASK                                     0xffffffff
#define DMA_8_DMA_ADDR_MLSC_LSB                                               0

/* ISP_DMA_10 */
#define DMA_10_DMA_ADDR_WDR_SUBIN_MASK                               0xffffffff
#define DMA_10_DMA_ADDR_WDR_SUBIN_LSB                                         0

/* ISP_DMA_12 */
#define DMA_12_DMA_ADDR_WDR_SUBOUT_MASK                              0xffffffff
#define DMA_12_DMA_ADDR_WDR_SUBOUT_LSB                                        0

/* ISP_DMA_14 */
#define DMA_14_DMA_ADDR_LCE_SUBIN_MASK                               0xffffffff
#define DMA_14_DMA_ADDR_LCE_SUBIN_LSB                                         0

/* ISP_DMA_16 */
#define DMA_16_DMA_ADDR_LCE_SUBOUT_MASK                              0xffffffff
#define DMA_16_DMA_ADDR_LCE_SUBOUT_LSB                                        0

/* ISP_DMA_18 */
#define DMA_18_DMA_ADDR_CNR_SUBIN_MASK                               0xffffffff
#define DMA_18_DMA_ADDR_CNR_SUBIN_LSB                                         0

/* ISP_DMA_20 */
#define DMA_20_DMA_ADDR_CNR_SUBOUT_MASK                              0xffffffff
#define DMA_20_DMA_ADDR_CNR_SUBOUT_LSB                                        0

/* ISP_DMA_22 */
#define DMA_22_DMA_ADDR_NR3D_Y_REF_IN_MASK                           0xffffffff
#define DMA_22_DMA_ADDR_NR3D_Y_REF_IN_LSB                                     0

/* ISP_DMA_24 */
#define DMA_24_DMA_ADDR_NR3D_Y_REF_OUT_MASK                          0xffffffff
#define DMA_24_DMA_ADDR_NR3D_Y_REF_OUT_LSB                                    0

/* ISP_DMA_26 */
#define DMA_26_DMA_ADDR_NR3D_UV_REF_IN_MASK                          0xffffffff
#define DMA_26_DMA_ADDR_NR3D_UV_REF_IN_LSB                                    0

/* ISP_DMA_28 */
#define DMA_28_DMA_ADDR_NR3D_UV_REF_OUT_MASK                         0xffffffff
#define DMA_28_DMA_ADDR_NR3D_UV_REF_OUT_LSB                                   0

/* ISP_DMA_30 */
#define DMA_30_DMA_ADDR_Y_OUT_MASK                                   0xffffffff
#define DMA_30_DMA_ADDR_Y_OUT_LSB                                             0

/* ISP_DMA_32 */
#define DMA_32_DMA_ADDR_UV_OUT_MASK                                  0xffffffff
#define DMA_32_DMA_ADDR_UV_OUT_LSB                                            0

/* ISP_DMA_34 */
#define DMA_34_DMA_ADDR_AWB_OUT_MASK                                 0xffffffff
#define DMA_34_DMA_ADDR_AWB_OUT_LSB                                           0

/* ISP_DMA_36 */
#define DMA_36_DMA_ADDR_AE_BA_MASK                                   0xffffffff
#define DMA_36_DMA_ADDR_AE_BA_LSB                                             0

/* ISP_DMA_38 */
#define DMA_38_DMA_ADDR_AE_HIST_MASK                                 0xffffffff
#define DMA_38_DMA_ADDR_AE_HIST_LSB                                           0

/* ISP_DMA_40 */
#define DMA_40_DMA_ADDR_AE2_BA_MASK                                  0xffffffff
#define DMA_40_DMA_ADDR_AE2_BA_LSB                                            0

/* ISP_DMA_42 */
#define DMA_42_DMA_ADDR_AE2_HIST_MASK                                0xffffffff
#define DMA_42_DMA_ADDR_AE2_HIST_LSB                                          0

/* ISP_DMA_44 */
#define DMA_44_DMA_ADDR_NR3D_MOTION_REF_IN_MASK                      0xffffffff
#define DMA_44_DMA_ADDR_NR3D_MOTION_REF_IN_LSB                                0

/* ISP_DMA_46 */
#define DMA_46_DMA_ADDR_NR3D_MOTION_CUR_OUT_MASK                     0xffffffff
#define DMA_46_DMA_ADDR_NR3D_MOTION_CUR_OUT_LSB                               0

/* ISP_DMA_47 */
#define DMA_47_DMA_ADDR_U_OUT_MASK                                   0xffffffff
#define DMA_47_DMA_ADDR_U_OUT_LSB                                             0

/* ISP_DMA_48 */
#define DMA_48_DMA_ADDR_V_OUT_MASK                                   0xffffffff
#define DMA_48_DMA_ADDR_V_OUT_LSB                                             0

/* ISP_DMA_49 */
#define DMA_49_DMA_ADDR_NR3D_U_REF_IN_MASK                           0xffffffff
#define DMA_49_DMA_ADDR_NR3D_U_REF_IN_LSB                                     0

/* ISP_DMA_50 */
#define DMA_50_DMA_ADDR_NR3D_V_REF_IN_MASK                           0xffffffff
#define DMA_50_DMA_ADDR_NR3D_V_REF_IN_LSB                                     0

/* ISP_DMA_51 */
#define DMA_51_DMA_ADDR_NR3D_U_REF_OUT_MASK                          0xffffffff
#define DMA_51_DMA_ADDR_NR3D_U_REF_OUT_LSB                                    0

/* ISP_DMA_52 */
#define DMA_52_DMA_ADDR_NR3D_V_REF_OUT_MASK                          0xffffffff
#define DMA_52_DMA_ADDR_NR3D_V_REF_OUT_LSB                                    0

/* ISP_FCURVE_DMA_WH */
#define FCURVE_DMA_WH_FCURVE_WIDTH_MASK                                  0x1fff
#define FCURVE_DMA_WH_FCURVE_WIDTH_LSB                                        0
#define FCURVE_DMA_WH_FCURVE_HEIGHT_MASK                             0x1fff0000
#define FCURVE_DMA_WH_FCURVE_HEIGHT_LSB                                      16

/* ISP_MLSC_DMA_WH */
#define MLSC_DMA_WH_MLSC_WIDTH_MASK                                      0x1fff
#define MLSC_DMA_WH_MLSC_WIDTH_LSB                                            0
#define MLSC_DMA_WH_MLSC_HEIGHT_MASK                                 0x1fff0000
#define MLSC_DMA_WH_MLSC_HEIGHT_LSB                                          16

/* ISP_WDR_DMA_WH */
#define WDR_DMA_WH_WDR_WIDTH_MASK                                        0x1fff
#define WDR_DMA_WH_WDR_WIDTH_LSB                                              0
#define WDR_DMA_WH_WDR_HEIGHT_MASK                                   0x1fff0000
#define WDR_DMA_WH_WDR_HEIGHT_LSB                                            16

/* ISP_LCE_DMA_WH */
#define LCE_DMA_WH_LCE_WIDTH_MASK                                        0x1fff
#define LCE_DMA_WH_LCE_WIDTH_LSB                                              0
#define LCE_DMA_WH_LCE_HEIGHT_MASK                                   0x1fff0000
#define LCE_DMA_WH_LCE_HEIGHT_LSB                                            16

/* ISP_CNR_DMA_WH */
#define CNR_DMA_WH_CNR_WIDTH_MASK                                        0x1fff
#define CNR_DMA_WH_CNR_WIDTH_LSB                                              0
#define CNR_DMA_WH_CNR_HEIGHT_MASK                                   0x1fff0000
#define CNR_DMA_WH_CNR_HEIGHT_LSB                                            16

/* ISP_HDR_0 */
#define HDR_0_HDR_WB1_COLORGAIN_R_MASK                                   0x1fff
#define HDR_0_HDR_WB1_COLORGAIN_R_LSB                                         0
#define HDR_0_HDR_WB1_COLORGAIN_GR_MASK                              0x1fff0000
#define HDR_0_HDR_WB1_COLORGAIN_GR_LSB                                       16

/* ISP_HDR_1 */
#define HDR_1_HDR_WB1_COLORGAIN_GB_MASK                                  0x1fff
#define HDR_1_HDR_WB1_COLORGAIN_GB_LSB                                        0
#define HDR_1_HDR_WB1_COLORGAIN_B_MASK                               0x1fff0000
#define HDR_1_HDR_WB1_COLORGAIN_B_LSB                                        16

/* ISP_HDR_2 */
#define HDR_2_HDR_WB1_OB_R_MASK                                           0xfff
#define HDR_2_HDR_WB1_OB_R_LSB                                                0
#define HDR_2_HDR_WB1_OB_GR_MASK                                      0xfff0000
#define HDR_2_HDR_WB1_OB_GR_LSB                                              16

/* ISP_HDR_3 */
#define HDR_3_HDR_WB1_OB_GB_MASK                                          0xfff
#define HDR_3_HDR_WB1_OB_GB_LSB                                               0
#define HDR_3_HDR_WB1_OB_B_MASK                                       0xfff0000
#define HDR_3_HDR_WB1_OB_B_LSB                                               16

/* ISP_HDR_4 */
#define HDR_4_HDR_WB2_COLORGAIN_R_MASK                                   0x1fff
#define HDR_4_HDR_WB2_COLORGAIN_R_LSB                                         0
#define HDR_4_HDR_WB2_COLORGAIN_GR_MASK                              0x1fff0000
#define HDR_4_HDR_WB2_COLORGAIN_GR_LSB                                       16

/* ISP_HDR_5 */
#define HDR_5_HDR_WB2_COLORGAIN_GB_MASK                                  0x1fff
#define HDR_5_HDR_WB2_COLORGAIN_GB_LSB                                        0
#define HDR_5_HDR_WB2_COLORGAIN_B_MASK                               0x1fff0000
#define HDR_5_HDR_WB2_COLORGAIN_B_LSB                                        16

/* ISP_HDR_6 */
#define HDR_6_HDR_WB2_OB_R_MASK                                           0xfff
#define HDR_6_HDR_WB2_OB_R_LSB                                                0
#define HDR_6_HDR_WB2_OB_GR_MASK                                      0xfff0000
#define HDR_6_HDR_WB2_OB_GR_LSB                                              16

/* ISP_HDR_7 */
#define HDR_7_HDR_WB2_OB_GB_MASK                                          0xfff
#define HDR_7_HDR_WB2_OB_GB_LSB                                               0
#define HDR_7_HDR_WB2_OB_B_MASK                                       0xfff0000
#define HDR_7_HDR_WB2_OB_B_LSB                                               16

/* ISP_HDR_8 */
#define HDR_8_FS_INDEX_SEL_MASK                                             0x1
#define HDR_8_FS_INDEX_SEL_LSB                                                0
#define HDR_8_FS_INDEX_WT_BAYER_MASK                                    0x1ff00
#define HDR_8_FS_INDEX_WT_BAYER_LSB                                           8

/* ISP_HDR_9 */
#define HDR_9_FS_GAINS_MASK                                              0x7fff
#define HDR_9_FS_GAINS_LSB                                                    0
#define HDR_9_FS_GAINL_MASK                                          0x7fff0000
#define HDR_9_FS_GAINL_LSB                                                   16

/* ISP_HDR_10 */
#define HDR_10_FS_LOW_THRES_MASK                                          0xfff
#define HDR_10_FS_LOW_THRES_LSB                                               0
#define HDR_10_FS_HIGH_THRES_MASK                                     0xfff0000
#define HDR_10_FS_HIGH_THRES_LSB                                             16

/* ISP_HDR_11 */
#define HDR_11_FS_WT_MIN_MASK                                             0x1ff
#define HDR_11_FS_WT_MIN_LSB                                                  0
#define HDR_11_FS_WT_MAX_MASK                                         0x1ff0000
#define HDR_11_FS_WT_MAX_LSB                                                 16

/* ISP_HDR_12 */
#define HDR_12_FS_WT_SLOPE_MASK                                         0xfffff
#define HDR_12_FS_WT_SLOPE_LSB                                                0

/* ISP_FCURVE_0 */
#define FCURVE_0_FCURVE_SUB_WIDTH_MASK                                     0x3f
#define FCURVE_0_FCURVE_SUB_WIDTH_LSB                                         0
#define FCURVE_0_FCURVE_SUB_HEIGHT_MASK                                0x3f0000
#define FCURVE_0_FCURVE_SUB_HEIGHT_LSB                                       16

/* ISP_FCURVE_1 */
#define FCURVE_1_FCURVE_BLK_STCS_FACT_H_MASK                           0xffffff
#define FCURVE_1_FCURVE_BLK_STCS_FACT_H_LSB                                   0

/* ISP_FCURVE_2 */
#define FCURVE_2_FCURVE_BLK_STCS_FACT_V_MASK                           0xffffff
#define FCURVE_2_FCURVE_BLK_STCS_FACT_V_LSB                                   0

/* ISP_FCURVE_3 */
#define FCURVE_3_FCURVE_SCALING_FACT_H_MASK                              0xffff
#define FCURVE_3_FCURVE_SCALING_FACT_H_LSB                                    0
#define FCURVE_3_FCURVE_SCALING_FACT_V_MASK                          0xffff0000
#define FCURVE_3_FCURVE_SCALING_FACT_V_LSB                                   16

/* ISP_FCURVE_4 */
#define FCURVE_4_FCURVE_BLK_SIZE_H_MASK                                   0x3ff
#define FCURVE_4_FCURVE_BLK_SIZE_H_LSB                                        0
#define FCURVE_4_FCURVE_BLK_SIZE_V_MASK                               0x3ff0000
#define FCURVE_4_FCURVE_BLK_SIZE_V_LSB                                       16

/* ISP_FCURVE_5 */
#define FCURVE_5_FCURVE_BLK_SIZE_NORM_DIV_MASK                          0xfffff
#define FCURVE_5_FCURVE_BLK_SIZE_NORM_DIV_LSB                                 0

/* ISP_FCURVE_6 */
#define FCURVE_6_FCURVE_SUBIN_FILT_W0_MASK                                  0xf
#define FCURVE_6_FCURVE_SUBIN_FILT_W0_LSB                                     0
#define FCURVE_6_FCURVE_SUBIN_FILT_W1_MASK                                 0xf0
#define FCURVE_6_FCURVE_SUBIN_FILT_W1_LSB                                     4
#define FCURVE_6_FCURVE_SUBIN_FILT_W2_MASK                                0xf00
#define FCURVE_6_FCURVE_SUBIN_FILT_W2_LSB                                     8
#define FCURVE_6_FCURVE_SUBIN_FILT_NORM_DIV_MASK                      0xfff0000
#define FCURVE_6_FCURVE_SUBIN_FILT_NORM_DIV_LSB                              16

/* ISP_FCURVE_7 */
#define FCURVE_7_FCURVE_STCS_GAIN_MASK                                     0xff
#define FCURVE_7_FCURVE_STCS_GAIN_LSB                                         0

/* ISP_FCURVE_8 */
#define FCURVE_8_FCURVE_WT_Y_LUT0_MASK                                     0xff
#define FCURVE_8_FCURVE_WT_Y_LUT0_LSB                                         0
#define FCURVE_8_FCURVE_WT_Y_LUT1_MASK                                   0xff00
#define FCURVE_8_FCURVE_WT_Y_LUT1_LSB                                         8
#define FCURVE_8_FCURVE_WT_Y_LUT2_MASK                                 0xff0000
#define FCURVE_8_FCURVE_WT_Y_LUT2_LSB                                        16
#define FCURVE_8_FCURVE_WT_Y_LUT3_MASK                               0xff000000
#define FCURVE_8_FCURVE_WT_Y_LUT3_LSB                                        24

/* ISP_FCURVE_9 */
#define FCURVE_9_FCURVE_WT_Y_LUT4_MASK                                     0xff
#define FCURVE_9_FCURVE_WT_Y_LUT4_LSB                                         0
#define FCURVE_9_FCURVE_WT_Y_LUT5_MASK                                   0xff00
#define FCURVE_9_FCURVE_WT_Y_LUT5_LSB                                         8
#define FCURVE_9_FCURVE_WT_Y_LUT6_MASK                                 0xff0000
#define FCURVE_9_FCURVE_WT_Y_LUT6_LSB                                        16
#define FCURVE_9_FCURVE_WT_Y_LUT7_MASK                               0xff000000
#define FCURVE_9_FCURVE_WT_Y_LUT7_LSB                                        24

/* ISP_FCURVE_10 */
#define FCURVE_10_FCURVE_WT_Y_LUT8_MASK                                    0xff
#define FCURVE_10_FCURVE_WT_Y_LUT8_LSB                                        0
#define FCURVE_10_FCURVE_WT_Y_LUT9_MASK                                  0xff00
#define FCURVE_10_FCURVE_WT_Y_LUT9_LSB                                        8
#define FCURVE_10_FCURVE_WT_Y_LUT10_MASK                               0xff0000
#define FCURVE_10_FCURVE_WT_Y_LUT10_LSB                                      16
#define FCURVE_10_FCURVE_WT_Y_LUT11_MASK                             0xff000000
#define FCURVE_10_FCURVE_WT_Y_LUT11_LSB                                      24

/* ISP_FCURVE_11 */
#define FCURVE_11_FCURVE_WT_Y_LUT12_MASK                                   0xff
#define FCURVE_11_FCURVE_WT_Y_LUT12_LSB                                       0
#define FCURVE_11_FCURVE_WT_Y_LUT13_MASK                                 0xff00
#define FCURVE_11_FCURVE_WT_Y_LUT13_LSB                                       8
#define FCURVE_11_FCURVE_WT_Y_LUT14_MASK                               0xff0000
#define FCURVE_11_FCURVE_WT_Y_LUT14_LSB                                      16
#define FCURVE_11_FCURVE_WT_Y_LUT15_MASK                             0xff000000
#define FCURVE_11_FCURVE_WT_Y_LUT15_LSB                                      24

/* ISP_FCURVE_12 */
#define FCURVE_12_FCURVE_WT_Y_LUT16_MASK                                   0xff
#define FCURVE_12_FCURVE_WT_Y_LUT16_LSB                                       0

/* ISP_FCURVE_13 */
#define FCURVE_13_FCURVE_BLK_BORDER_U_MASK                                0x3ff
#define FCURVE_13_FCURVE_BLK_BORDER_U_LSB                                     0
#define FCURVE_13_FCURVE_BLK_CORNER_UL_MASK                          0x3ffffc00
#define FCURVE_13_FCURVE_BLK_CORNER_UL_LSB                                   10

/* ISP_FCURVE_14 */
#define FCURVE_14_FCURVE_BLK_BORDER_D_MASK                                0x3ff
#define FCURVE_14_FCURVE_BLK_BORDER_D_LSB                                     0
#define FCURVE_14_FCURVE_BLK_CORNER_UR_MASK                          0x3ffffc00
#define FCURVE_14_FCURVE_BLK_CORNER_UR_LSB                                   10

/* ISP_FCURVE_15 */
#define FCURVE_15_FCURVE_BLK_BORDER_L_MASK                                0x3ff
#define FCURVE_15_FCURVE_BLK_BORDER_L_LSB                                     0
#define FCURVE_15_FCURVE_BLK_CORNER_DL_MASK                          0x3ffffc00
#define FCURVE_15_FCURVE_BLK_CORNER_DL_LSB                                   10

/* ISP_FCURVE_16 */
#define FCURVE_16_FCURVE_BLK_BORDER_R_MASK                                0x3ff
#define FCURVE_16_FCURVE_BLK_BORDER_R_LSB                                     0
#define FCURVE_16_FCURVE_BLK_CORNER_DR_MASK                          0x3ffffc00
#define FCURVE_16_FCURVE_BLK_CORNER_DR_LSB                                   10

/* ISP_FCURVE_17 */
#define FCURVE_17_TONECURVE_TABLE0_MASK                                   0xfff
#define FCURVE_17_TONECURVE_TABLE0_LSB                                        0
#define FCURVE_17_TONECURVE_TABLE1_MASK                               0xfff0000
#define FCURVE_17_TONECURVE_TABLE1_LSB                                       16

/* ISP_FCURVE_18 */
#define FCURVE_18_TONECURVE_TABLE2_MASK                                   0xfff
#define FCURVE_18_TONECURVE_TABLE2_LSB                                        0
#define FCURVE_18_TONECURVE_TABLE3_MASK                               0xfff0000
#define FCURVE_18_TONECURVE_TABLE3_LSB                                       16

/* ISP_FCURVE_19 */
#define FCURVE_19_TONECURVE_TABLE4_MASK                                   0xfff
#define FCURVE_19_TONECURVE_TABLE4_LSB                                        0
#define FCURVE_19_TONECURVE_TABLE5_MASK                               0xfff0000
#define FCURVE_19_TONECURVE_TABLE5_LSB                                       16

/* ISP_FCURVE_20 */
#define FCURVE_20_TONECURVE_TABLE6_MASK                                   0xfff
#define FCURVE_20_TONECURVE_TABLE6_LSB                                        0
#define FCURVE_20_TONECURVE_TABLE7_MASK                               0xfff0000
#define FCURVE_20_TONECURVE_TABLE7_LSB                                       16

/* ISP_FCURVE_21 */
#define FCURVE_21_TONECURVE_TABLE8_MASK                                   0xfff
#define FCURVE_21_TONECURVE_TABLE8_LSB                                        0
#define FCURVE_21_TONECURVE_TABLE9_MASK                               0xfff0000
#define FCURVE_21_TONECURVE_TABLE9_LSB                                       16

/* ISP_FCURVE_22 */
#define FCURVE_22_TONECURVE_TABLE10_MASK                                  0xfff
#define FCURVE_22_TONECURVE_TABLE10_LSB                                       0
#define FCURVE_22_TONECURVE_TABLE11_MASK                              0xfff0000
#define FCURVE_22_TONECURVE_TABLE11_LSB                                      16

/* ISP_FCURVE_23 */
#define FCURVE_23_TONECURVE_TABLE12_MASK                                  0xfff
#define FCURVE_23_TONECURVE_TABLE12_LSB                                       0
#define FCURVE_23_TONECURVE_TABLE13_MASK                              0xfff0000
#define FCURVE_23_TONECURVE_TABLE13_LSB                                      16

/* ISP_FCURVE_24 */
#define FCURVE_24_TONECURVE_TABLE14_MASK                                  0xfff
#define FCURVE_24_TONECURVE_TABLE14_LSB                                       0
#define FCURVE_24_TONECURVE_TABLE15_MASK                              0xfff0000
#define FCURVE_24_TONECURVE_TABLE15_LSB                                      16

/* ISP_FCURVE_25 */
#define FCURVE_25_TONECURVE_TABLE16_MASK                                  0xfff
#define FCURVE_25_TONECURVE_TABLE16_LSB                                       0
#define FCURVE_25_TONECURVE_TABLE17_MASK                              0xfff0000
#define FCURVE_25_TONECURVE_TABLE17_LSB                                      16

/* ISP_FCURVE_26 */
#define FCURVE_26_TONECURVE_TABLE18_MASK                                  0xfff
#define FCURVE_26_TONECURVE_TABLE18_LSB                                       0
#define FCURVE_26_TONECURVE_TABLE19_MASK                              0xfff0000
#define FCURVE_26_TONECURVE_TABLE19_LSB                                      16

/* ISP_FCURVE_27 */
#define FCURVE_27_TONECURVE_TABLE20_MASK                                  0xfff
#define FCURVE_27_TONECURVE_TABLE20_LSB                                       0
#define FCURVE_27_TONECURVE_TABLE21_MASK                              0xfff0000
#define FCURVE_27_TONECURVE_TABLE21_LSB                                      16

/* ISP_FCURVE_28 */
#define FCURVE_28_TONECURVE_TABLE22_MASK                                  0xfff
#define FCURVE_28_TONECURVE_TABLE22_LSB                                       0
#define FCURVE_28_TONECURVE_TABLE23_MASK                              0xfff0000
#define FCURVE_28_TONECURVE_TABLE23_LSB                                      16

/* ISP_FCURVE_29 */
#define FCURVE_29_TONECURVE_TABLE24_MASK                                  0xfff
#define FCURVE_29_TONECURVE_TABLE24_LSB                                       0
#define FCURVE_29_TONECURVE_TABLE25_MASK                              0xfff0000
#define FCURVE_29_TONECURVE_TABLE25_LSB                                      16

/* ISP_FCURVE_30 */
#define FCURVE_30_TONECURVE_TABLE26_MASK                                  0xfff
#define FCURVE_30_TONECURVE_TABLE26_LSB                                       0
#define FCURVE_30_TONECURVE_TABLE27_MASK                              0xfff0000
#define FCURVE_30_TONECURVE_TABLE27_LSB                                      16

/* ISP_FCURVE_31 */
#define FCURVE_31_TONECURVE_TABLE28_MASK                                  0xfff
#define FCURVE_31_TONECURVE_TABLE28_LSB                                       0
#define FCURVE_31_TONECURVE_TABLE29_MASK                              0xfff0000
#define FCURVE_31_TONECURVE_TABLE29_LSB                                      16

/* ISP_FCURVE_32 */
#define FCURVE_32_TONECURVE_TABLE30_MASK                                  0xfff
#define FCURVE_32_TONECURVE_TABLE30_LSB                                       0
#define FCURVE_32_TONECURVE_TABLE31_MASK                              0xfff0000
#define FCURVE_32_TONECURVE_TABLE31_LSB                                      16

/* ISP_FCURVE_33 */
#define FCURVE_33_TONECURVE_TABLE32_MASK                                  0xfff
#define FCURVE_33_TONECURVE_TABLE32_LSB                                       0
#define FCURVE_33_TONECURVE_TABLE33_MASK                              0xfff0000
#define FCURVE_33_TONECURVE_TABLE33_LSB                                      16

/* ISP_FCURVE_34 */
#define FCURVE_34_TONECURVE_TABLE34_MASK                                  0xfff
#define FCURVE_34_TONECURVE_TABLE34_LSB                                       0
#define FCURVE_34_TONECURVE_TABLE35_MASK                              0xfff0000
#define FCURVE_34_TONECURVE_TABLE35_LSB                                      16

/* ISP_FCURVE_35 */
#define FCURVE_35_TONECURVE_TABLE36_MASK                                  0xfff
#define FCURVE_35_TONECURVE_TABLE36_LSB                                       0
#define FCURVE_35_TONECURVE_TABLE37_MASK                              0xfff0000
#define FCURVE_35_TONECURVE_TABLE37_LSB                                      16

/* ISP_FCURVE_36 */
#define FCURVE_36_TONECURVE_TABLE38_MASK                                  0xfff
#define FCURVE_36_TONECURVE_TABLE38_LSB                                       0
#define FCURVE_36_TONECURVE_TABLE39_MASK                              0xfff0000
#define FCURVE_36_TONECURVE_TABLE39_LSB                                      16

/* ISP_FCURVE_37 */
#define FCURVE_37_TONECURVE_TABLE40_MASK                                  0xfff
#define FCURVE_37_TONECURVE_TABLE40_LSB                                       0
#define FCURVE_37_TONECURVE_TABLE41_MASK                              0xfff0000
#define FCURVE_37_TONECURVE_TABLE41_LSB                                      16

/* ISP_FCURVE_38 */
#define FCURVE_38_TONECURVE_TABLE42_MASK                                  0xfff
#define FCURVE_38_TONECURVE_TABLE42_LSB                                       0
#define FCURVE_38_TONECURVE_TABLE43_MASK                              0xfff0000
#define FCURVE_38_TONECURVE_TABLE43_LSB                                      16

/* ISP_FCURVE_39 */
#define FCURVE_39_TONECURVE_TABLE44_MASK                                  0xfff
#define FCURVE_39_TONECURVE_TABLE44_LSB                                       0
#define FCURVE_39_TONECURVE_TABLE45_MASK                              0xfff0000
#define FCURVE_39_TONECURVE_TABLE45_LSB                                      16

/* ISP_FCURVE_40 */
#define FCURVE_40_TONECURVE_TABLE46_MASK                                  0xfff
#define FCURVE_40_TONECURVE_TABLE46_LSB                                       0
#define FCURVE_40_TONECURVE_TABLE47_MASK                              0xfff0000
#define FCURVE_40_TONECURVE_TABLE47_LSB                                      16

/* ISP_FCURVE_41 */
#define FCURVE_41_TONECURVE_TABLE48_MASK                                  0xfff
#define FCURVE_41_TONECURVE_TABLE48_LSB                                       0
#define FCURVE_41_TONECURVE_TABLE49_MASK                              0xfff0000
#define FCURVE_41_TONECURVE_TABLE49_LSB                                      16

/* ISP_FCURVE_42 */
#define FCURVE_42_TONECURVE_TABLE50_MASK                                  0xfff
#define FCURVE_42_TONECURVE_TABLE50_LSB                                       0
#define FCURVE_42_TONECURVE_TABLE51_MASK                              0xfff0000
#define FCURVE_42_TONECURVE_TABLE51_LSB                                      16

/* ISP_FCURVE_43 */
#define FCURVE_43_TONECURVE_TABLE52_MASK                                  0xfff
#define FCURVE_43_TONECURVE_TABLE52_LSB                                       0
#define FCURVE_43_TONECURVE_TABLE53_MASK                              0xfff0000
#define FCURVE_43_TONECURVE_TABLE53_LSB                                      16

/* ISP_FCURVE_44 */
#define FCURVE_44_TONECURVE_TABLE54_MASK                                  0xfff
#define FCURVE_44_TONECURVE_TABLE54_LSB                                       0
#define FCURVE_44_TONECURVE_TABLE55_MASK                              0xfff0000
#define FCURVE_44_TONECURVE_TABLE55_LSB                                      16

/* ISP_FCURVE_45 */
#define FCURVE_45_TONECURVE_TABLE56_MASK                                  0xfff
#define FCURVE_45_TONECURVE_TABLE56_LSB                                       0
#define FCURVE_45_TONECURVE_TABLE57_MASK                              0xfff0000
#define FCURVE_45_TONECURVE_TABLE57_LSB                                      16

/* ISP_FCURVE_46 */
#define FCURVE_46_TONECURVE_TABLE58_MASK                                  0xfff
#define FCURVE_46_TONECURVE_TABLE58_LSB                                       0
#define FCURVE_46_TONECURVE_TABLE59_MASK                              0xfff0000
#define FCURVE_46_TONECURVE_TABLE59_LSB                                      16

/* ISP_FCURVE_47 */
#define FCURVE_47_TONECURVE_TABLE60_MASK                                  0xfff
#define FCURVE_47_TONECURVE_TABLE60_LSB                                       0
#define FCURVE_47_TONECURVE_TABLE61_MASK                              0xfff0000
#define FCURVE_47_TONECURVE_TABLE61_LSB                                      16

/* ISP_FCURVE_48 */
#define FCURVE_48_TONECURVE_TABLE62_MASK                                  0xfff
#define FCURVE_48_TONECURVE_TABLE62_LSB                                       0
#define FCURVE_48_TONECURVE_TABLE63_MASK                              0xfff0000
#define FCURVE_48_TONECURVE_TABLE63_LSB                                      16

/* ISP_FCURVE_49 */
#define FCURVE_49_TONECURVE_TABLE64_MASK                                  0xfff
#define FCURVE_49_TONECURVE_TABLE64_LSB                                       0
#define FCURVE_49_TONECURVE_TABLE65_MASK                              0xfff0000
#define FCURVE_49_TONECURVE_TABLE65_LSB                                      16

/* ISP_FCURVE_50 */
#define FCURVE_50_TONECURVE_TABLE66_MASK                                  0xfff
#define FCURVE_50_TONECURVE_TABLE66_LSB                                       0
#define FCURVE_50_TONECURVE_TABLE67_MASK                              0xfff0000
#define FCURVE_50_TONECURVE_TABLE67_LSB                                      16

/* ISP_FCURVE_51 */
#define FCURVE_51_TONECURVE_TABLE68_MASK                                  0xfff
#define FCURVE_51_TONECURVE_TABLE68_LSB                                       0
#define FCURVE_51_TONECURVE_TABLE69_MASK                              0xfff0000
#define FCURVE_51_TONECURVE_TABLE69_LSB                                      16

/* ISP_FCURVE_52 */
#define FCURVE_52_TONECURVE_TABLE70_MASK                                  0xfff
#define FCURVE_52_TONECURVE_TABLE70_LSB                                       0
#define FCURVE_52_TONECURVE_TABLE71_MASK                              0xfff0000
#define FCURVE_52_TONECURVE_TABLE71_LSB                                      16

/* ISP_FCURVE_53 */
#define FCURVE_53_TONECURVE_TABLE72_MASK                                  0xfff
#define FCURVE_53_TONECURVE_TABLE72_LSB                                       0
#define FCURVE_53_TONECURVE_TABLE73_MASK                              0xfff0000
#define FCURVE_53_TONECURVE_TABLE73_LSB                                      16

/* ISP_FCURVE_54 */
#define FCURVE_54_TONECURVE_TABLE74_MASK                                  0xfff
#define FCURVE_54_TONECURVE_TABLE74_LSB                                       0
#define FCURVE_54_TONECURVE_TABLE75_MASK                              0xfff0000
#define FCURVE_54_TONECURVE_TABLE75_LSB                                      16

/* ISP_FCURVE_55 */
#define FCURVE_55_TONECURVE_TABLE76_MASK                                  0xfff
#define FCURVE_55_TONECURVE_TABLE76_LSB                                       0
#define FCURVE_55_TONECURVE_TABLE77_MASK                              0xfff0000
#define FCURVE_55_TONECURVE_TABLE77_LSB                                      16

/* ISP_FCURVE_56 */
#define FCURVE_56_TONECURVE_TABLE78_MASK                                  0xfff
#define FCURVE_56_TONECURVE_TABLE78_LSB                                       0
#define FCURVE_56_TONECURVE_TABLE79_MASK                              0xfff0000
#define FCURVE_56_TONECURVE_TABLE79_LSB                                      16

/* ISP_FCURVE_57 */
#define FCURVE_57_TONECURVE_TABLE80_MASK                                  0xfff
#define FCURVE_57_TONECURVE_TABLE80_LSB                                       0
#define FCURVE_57_TONECURVE_TABLE81_MASK                              0xfff0000
#define FCURVE_57_TONECURVE_TABLE81_LSB                                      16

/* ISP_FCURVE_58 */
#define FCURVE_58_TONECURVE_TABLE82_MASK                                  0xfff
#define FCURVE_58_TONECURVE_TABLE82_LSB                                       0
#define FCURVE_58_TONECURVE_TABLE83_MASK                              0xfff0000
#define FCURVE_58_TONECURVE_TABLE83_LSB                                      16

/* ISP_FCURVE_59 */
#define FCURVE_59_TONECURVE_TABLE84_MASK                                  0xfff
#define FCURVE_59_TONECURVE_TABLE84_LSB                                       0
#define FCURVE_59_TONECURVE_TABLE85_MASK                              0xfff0000
#define FCURVE_59_TONECURVE_TABLE85_LSB                                      16

/* ISP_FCURVE_60 */
#define FCURVE_60_TONECURVE_TABLE86_MASK                                  0xfff
#define FCURVE_60_TONECURVE_TABLE86_LSB                                       0
#define FCURVE_60_TONECURVE_TABLE87_MASK                              0xfff0000
#define FCURVE_60_TONECURVE_TABLE87_LSB                                      16

/* ISP_FCURVE_61 */
#define FCURVE_61_TONECURVE_TABLE88_MASK                                  0xfff
#define FCURVE_61_TONECURVE_TABLE88_LSB                                       0
#define FCURVE_61_TONECURVE_TABLE89_MASK                              0xfff0000
#define FCURVE_61_TONECURVE_TABLE89_LSB                                      16

/* ISP_FCURVE_62 */
#define FCURVE_62_TONECURVE_TABLE90_MASK                                  0xfff
#define FCURVE_62_TONECURVE_TABLE90_LSB                                       0
#define FCURVE_62_TONECURVE_TABLE91_MASK                              0xfff0000
#define FCURVE_62_TONECURVE_TABLE91_LSB                                      16

/* ISP_FCURVE_63 */
#define FCURVE_63_TONECURVE_TABLE92_MASK                                  0xfff
#define FCURVE_63_TONECURVE_TABLE92_LSB                                       0
#define FCURVE_63_TONECURVE_TABLE93_MASK                              0xfff0000
#define FCURVE_63_TONECURVE_TABLE93_LSB                                      16

/* ISP_FCURVE_64 */
#define FCURVE_64_TONECURVE_TABLE94_MASK                                  0xfff
#define FCURVE_64_TONECURVE_TABLE94_LSB                                       0
#define FCURVE_64_TONECURVE_TABLE95_MASK                              0xfff0000
#define FCURVE_64_TONECURVE_TABLE95_LSB                                      16

/* ISP_FCURVE_65 */
#define FCURVE_65_TONECURVE_TABLE96_MASK                                  0xfff
#define FCURVE_65_TONECURVE_TABLE96_LSB                                       0
#define FCURVE_65_TONECURVE_TABLE97_MASK                              0xfff0000
#define FCURVE_65_TONECURVE_TABLE97_LSB                                      16

/* ISP_FCURVE_66 */
#define FCURVE_66_TONECURVE_TABLE98_MASK                                  0xfff
#define FCURVE_66_TONECURVE_TABLE98_LSB                                       0
#define FCURVE_66_TONECURVE_TABLE99_MASK                              0xfff0000
#define FCURVE_66_TONECURVE_TABLE99_LSB                                      16

/* ISP_FCURVE_67 */
#define FCURVE_67_TONECURVE_TABLE100_MASK                                 0xfff
#define FCURVE_67_TONECURVE_TABLE100_LSB                                      0
#define FCURVE_67_TONECURVE_TABLE101_MASK                             0xfff0000
#define FCURVE_67_TONECURVE_TABLE101_LSB                                     16

/* ISP_FCURVE_68 */
#define FCURVE_68_TONECURVE_TABLE102_MASK                                 0xfff
#define FCURVE_68_TONECURVE_TABLE102_LSB                                      0
#define FCURVE_68_TONECURVE_TABLE103_MASK                             0xfff0000
#define FCURVE_68_TONECURVE_TABLE103_LSB                                     16

/* ISP_FCURVE_69 */
#define FCURVE_69_TONECURVE_TABLE104_MASK                                 0xfff
#define FCURVE_69_TONECURVE_TABLE104_LSB                                      0
#define FCURVE_69_TONECURVE_TABLE105_MASK                             0xfff0000
#define FCURVE_69_TONECURVE_TABLE105_LSB                                     16

/* ISP_FCURVE_70 */
#define FCURVE_70_TONECURVE_TABLE106_MASK                                 0xfff
#define FCURVE_70_TONECURVE_TABLE106_LSB                                      0
#define FCURVE_70_TONECURVE_TABLE107_MASK                             0xfff0000
#define FCURVE_70_TONECURVE_TABLE107_LSB                                     16

/* ISP_FCURVE_71 */
#define FCURVE_71_TONECURVE_TABLE108_MASK                                 0xfff
#define FCURVE_71_TONECURVE_TABLE108_LSB                                      0
#define FCURVE_71_TONECURVE_TABLE109_MASK                             0xfff0000
#define FCURVE_71_TONECURVE_TABLE109_LSB                                     16

/* ISP_FCURVE_72 */
#define FCURVE_72_TONECURVE_TABLE110_MASK                                 0xfff
#define FCURVE_72_TONECURVE_TABLE110_LSB                                      0
#define FCURVE_72_TONECURVE_TABLE111_MASK                             0xfff0000
#define FCURVE_72_TONECURVE_TABLE111_LSB                                     16

/* ISP_FCURVE_73 */
#define FCURVE_73_TONECURVE_TABLE112_MASK                                 0xfff
#define FCURVE_73_TONECURVE_TABLE112_LSB                                      0
#define FCURVE_73_TONECURVE_TABLE113_MASK                             0xfff0000
#define FCURVE_73_TONECURVE_TABLE113_LSB                                     16

/* ISP_FCURVE_74 */
#define FCURVE_74_TONECURVE_TABLE114_MASK                                 0xfff
#define FCURVE_74_TONECURVE_TABLE114_LSB                                      0
#define FCURVE_74_TONECURVE_TABLE115_MASK                             0xfff0000
#define FCURVE_74_TONECURVE_TABLE115_LSB                                     16

/* ISP_FCURVE_75 */
#define FCURVE_75_TONECURVE_TABLE116_MASK                                 0xfff
#define FCURVE_75_TONECURVE_TABLE116_LSB                                      0
#define FCURVE_75_TONECURVE_TABLE117_MASK                             0xfff0000
#define FCURVE_75_TONECURVE_TABLE117_LSB                                     16

/* ISP_FCURVE_76 */
#define FCURVE_76_TONECURVE_TABLE118_MASK                                 0xfff
#define FCURVE_76_TONECURVE_TABLE118_LSB                                      0
#define FCURVE_76_TONECURVE_TABLE119_MASK                             0xfff0000
#define FCURVE_76_TONECURVE_TABLE119_LSB                                     16

/* ISP_FCURVE_77 */
#define FCURVE_77_TONECURVE_TABLE120_MASK                                 0xfff
#define FCURVE_77_TONECURVE_TABLE120_LSB                                      0
#define FCURVE_77_TONECURVE_TABLE121_MASK                             0xfff0000
#define FCURVE_77_TONECURVE_TABLE121_LSB                                     16

/* ISP_FCURVE_78 */
#define FCURVE_78_TONECURVE_TABLE122_MASK                                 0xfff
#define FCURVE_78_TONECURVE_TABLE122_LSB                                      0
#define FCURVE_78_TONECURVE_TABLE123_MASK                             0xfff0000
#define FCURVE_78_TONECURVE_TABLE123_LSB                                     16

/* ISP_FCURVE_79 */
#define FCURVE_79_TONECURVE_TABLE124_MASK                                 0xfff
#define FCURVE_79_TONECURVE_TABLE124_LSB                                      0
#define FCURVE_79_TONECURVE_TABLE125_MASK                             0xfff0000
#define FCURVE_79_TONECURVE_TABLE125_LSB                                     16

/* ISP_FCURVE_80 */
#define FCURVE_80_TONECURVE_TABLE126_MASK                                 0xfff
#define FCURVE_80_TONECURVE_TABLE126_LSB                                      0
#define FCURVE_80_TONECURVE_TABLE127_MASK                             0xfff0000
#define FCURVE_80_TONECURVE_TABLE127_LSB                                     16

/* ISP_FCURVE_81 */
#define FCURVE_81_TONECURVE_TABLE128_MASK                                 0xfff
#define FCURVE_81_TONECURVE_TABLE128_LSB                                      0
#define FCURVE_81_TONECURVE_TABLE129_MASK                             0xfff0000
#define FCURVE_81_TONECURVE_TABLE129_LSB                                     16

/* ISP_FCURVE_82 */
#define FCURVE_82_TONECURVE_TABLE130_MASK                                 0xfff
#define FCURVE_82_TONECURVE_TABLE130_LSB                                      0
#define FCURVE_82_TONECURVE_TABLE131_MASK                             0xfff0000
#define FCURVE_82_TONECURVE_TABLE131_LSB                                     16

/* ISP_FCURVE_83 */
#define FCURVE_83_TONECURVE_TABLE132_MASK                                 0xfff
#define FCURVE_83_TONECURVE_TABLE132_LSB                                      0
#define FCURVE_83_TONECURVE_TABLE133_MASK                             0xfff0000
#define FCURVE_83_TONECURVE_TABLE133_LSB                                     16

/* ISP_FCURVE_84 */
#define FCURVE_84_TONECURVE_TABLE134_MASK                                 0xfff
#define FCURVE_84_TONECURVE_TABLE134_LSB                                      0
#define FCURVE_84_TONECURVE_TABLE135_MASK                             0xfff0000
#define FCURVE_84_TONECURVE_TABLE135_LSB                                     16

/* ISP_FCURVE_85 */
#define FCURVE_85_TONECURVE_TABLE136_MASK                                 0xfff
#define FCURVE_85_TONECURVE_TABLE136_LSB                                      0
#define FCURVE_85_TONECURVE_TABLE137_MASK                             0xfff0000
#define FCURVE_85_TONECURVE_TABLE137_LSB                                     16

/* ISP_FCURVE_86 */
#define FCURVE_86_TONECURVE_TABLE138_MASK                                 0xfff
#define FCURVE_86_TONECURVE_TABLE138_LSB                                      0
#define FCURVE_86_TONECURVE_TABLE139_MASK                             0xfff0000
#define FCURVE_86_TONECURVE_TABLE139_LSB                                     16

/* ISP_FCURVE_87 */
#define FCURVE_87_TONECURVE_TABLE140_MASK                                 0xfff
#define FCURVE_87_TONECURVE_TABLE140_LSB                                      0
#define FCURVE_87_TONECURVE_TABLE141_MASK                             0xfff0000
#define FCURVE_87_TONECURVE_TABLE141_LSB                                     16

/* ISP_FCURVE_88 */
#define FCURVE_88_TONECURVE_TABLE142_MASK                                 0xfff
#define FCURVE_88_TONECURVE_TABLE142_LSB                                      0
#define FCURVE_88_TONECURVE_TABLE143_MASK                             0xfff0000
#define FCURVE_88_TONECURVE_TABLE143_LSB                                     16

/* ISP_FCURVE_89 */
#define FCURVE_89_TONECURVE_TABLE144_MASK                                 0xfff
#define FCURVE_89_TONECURVE_TABLE144_LSB                                      0
#define FCURVE_89_TONECURVE_TABLE145_MASK                             0xfff0000
#define FCURVE_89_TONECURVE_TABLE145_LSB                                     16

/* ISP_FCURVE_90 */
#define FCURVE_90_TONECURVE_TABLE146_MASK                                 0xfff
#define FCURVE_90_TONECURVE_TABLE146_LSB                                      0
#define FCURVE_90_TONECURVE_TABLE147_MASK                             0xfff0000
#define FCURVE_90_TONECURVE_TABLE147_LSB                                     16

/* ISP_FCURVE_91 */
#define FCURVE_91_TONECURVE_TABLE148_MASK                                 0xfff
#define FCURVE_91_TONECURVE_TABLE148_LSB                                      0
#define FCURVE_91_TONECURVE_TABLE149_MASK                             0xfff0000
#define FCURVE_91_TONECURVE_TABLE149_LSB                                     16

/* ISP_FCURVE_92 */
#define FCURVE_92_TONECURVE_TABLE150_MASK                                 0xfff
#define FCURVE_92_TONECURVE_TABLE150_LSB                                      0
#define FCURVE_92_TONECURVE_TABLE151_MASK                             0xfff0000
#define FCURVE_92_TONECURVE_TABLE151_LSB                                     16

/* ISP_FCURVE_93 */
#define FCURVE_93_TONECURVE_TABLE152_MASK                                 0xfff
#define FCURVE_93_TONECURVE_TABLE152_LSB                                      0
#define FCURVE_93_TONECURVE_TABLE153_MASK                             0xfff0000
#define FCURVE_93_TONECURVE_TABLE153_LSB                                     16

/* ISP_FCURVE_94 */
#define FCURVE_94_TONECURVE_TABLE154_MASK                                 0xfff
#define FCURVE_94_TONECURVE_TABLE154_LSB                                      0
#define FCURVE_94_TONECURVE_TABLE155_MASK                             0xfff0000
#define FCURVE_94_TONECURVE_TABLE155_LSB                                     16

/* ISP_FCURVE_95 */
#define FCURVE_95_TONECURVE_TABLE156_MASK                                 0xfff
#define FCURVE_95_TONECURVE_TABLE156_LSB                                      0
#define FCURVE_95_TONECURVE_TABLE157_MASK                             0xfff0000
#define FCURVE_95_TONECURVE_TABLE157_LSB                                     16

/* ISP_FCURVE_96 */
#define FCURVE_96_TONECURVE_TABLE158_MASK                                 0xfff
#define FCURVE_96_TONECURVE_TABLE158_LSB                                      0
#define FCURVE_96_TONECURVE_TABLE159_MASK                             0xfff0000
#define FCURVE_96_TONECURVE_TABLE159_LSB                                     16

/* ISP_FCURVE_97 */
#define FCURVE_97_TONECURVE_TABLE160_MASK                                 0xfff
#define FCURVE_97_TONECURVE_TABLE160_LSB                                      0
#define FCURVE_97_TONECURVE_TABLE161_MASK                             0xfff0000
#define FCURVE_97_TONECURVE_TABLE161_LSB                                     16

/* ISP_FCURVE_98 */
#define FCURVE_98_TONECURVE_TABLE162_MASK                                 0xfff
#define FCURVE_98_TONECURVE_TABLE162_LSB                                      0
#define FCURVE_98_TONECURVE_TABLE163_MASK                             0xfff0000
#define FCURVE_98_TONECURVE_TABLE163_LSB                                     16

/* ISP_FCURVE_99 */
#define FCURVE_99_TONECURVE_TABLE164_MASK                                 0xfff
#define FCURVE_99_TONECURVE_TABLE164_LSB                                      0
#define FCURVE_99_TONECURVE_TABLE165_MASK                             0xfff0000
#define FCURVE_99_TONECURVE_TABLE165_LSB                                     16

/* ISP_FCURVE_100 */
#define FCURVE_100_TONECURVE_TABLE166_MASK                                0xfff
#define FCURVE_100_TONECURVE_TABLE166_LSB                                     0
#define FCURVE_100_TONECURVE_TABLE167_MASK                            0xfff0000
#define FCURVE_100_TONECURVE_TABLE167_LSB                                    16

/* ISP_FCURVE_101 */
#define FCURVE_101_TONECURVE_TABLE168_MASK                                0xfff
#define FCURVE_101_TONECURVE_TABLE168_LSB                                     0
#define FCURVE_101_TONECURVE_TABLE169_MASK                            0xfff0000
#define FCURVE_101_TONECURVE_TABLE169_LSB                                    16

/* ISP_FCURVE_102 */
#define FCURVE_102_TONECURVE_TABLE170_MASK                                0xfff
#define FCURVE_102_TONECURVE_TABLE170_LSB                                     0
#define FCURVE_102_TONECURVE_TABLE171_MASK                            0xfff0000
#define FCURVE_102_TONECURVE_TABLE171_LSB                                    16

/* ISP_FCURVE_103 */
#define FCURVE_103_TONECURVE_TABLE172_MASK                                0xfff
#define FCURVE_103_TONECURVE_TABLE172_LSB                                     0
#define FCURVE_103_TONECURVE_TABLE173_MASK                            0xfff0000
#define FCURVE_103_TONECURVE_TABLE173_LSB                                    16

/* ISP_FCURVE_104 */
#define FCURVE_104_TONECURVE_TABLE174_MASK                                0xfff
#define FCURVE_104_TONECURVE_TABLE174_LSB                                     0
#define FCURVE_104_TONECURVE_TABLE175_MASK                            0xfff0000
#define FCURVE_104_TONECURVE_TABLE175_LSB                                    16

/* ISP_FCURVE_105 */
#define FCURVE_105_TONECURVE_TABLE176_MASK                                0xfff
#define FCURVE_105_TONECURVE_TABLE176_LSB                                     0
#define FCURVE_105_TONECURVE_TABLE177_MASK                            0xfff0000
#define FCURVE_105_TONECURVE_TABLE177_LSB                                    16

/* ISP_FCURVE_106 */
#define FCURVE_106_TONECURVE_TABLE178_MASK                                0xfff
#define FCURVE_106_TONECURVE_TABLE178_LSB                                     0
#define FCURVE_106_TONECURVE_TABLE179_MASK                            0xfff0000
#define FCURVE_106_TONECURVE_TABLE179_LSB                                    16

/* ISP_FCURVE_107 */
#define FCURVE_107_TONECURVE_TABLE180_MASK                                0xfff
#define FCURVE_107_TONECURVE_TABLE180_LSB                                     0
#define FCURVE_107_TONECURVE_TABLE181_MASK                            0xfff0000
#define FCURVE_107_TONECURVE_TABLE181_LSB                                    16

/* ISP_FCURVE_108 */
#define FCURVE_108_TONECURVE_TABLE182_MASK                                0xfff
#define FCURVE_108_TONECURVE_TABLE182_LSB                                     0
#define FCURVE_108_TONECURVE_TABLE183_MASK                            0xfff0000
#define FCURVE_108_TONECURVE_TABLE183_LSB                                    16

/* ISP_FCURVE_109 */
#define FCURVE_109_TONECURVE_TABLE184_MASK                                0xfff
#define FCURVE_109_TONECURVE_TABLE184_LSB                                     0
#define FCURVE_109_TONECURVE_TABLE185_MASK                            0xfff0000
#define FCURVE_109_TONECURVE_TABLE185_LSB                                    16

/* ISP_FCURVE_110 */
#define FCURVE_110_TONECURVE_TABLE186_MASK                                0xfff
#define FCURVE_110_TONECURVE_TABLE186_LSB                                     0
#define FCURVE_110_TONECURVE_TABLE187_MASK                            0xfff0000
#define FCURVE_110_TONECURVE_TABLE187_LSB                                    16

/* ISP_FCURVE_111 */
#define FCURVE_111_TONECURVE_TABLE188_MASK                                0xfff
#define FCURVE_111_TONECURVE_TABLE188_LSB                                     0
#define FCURVE_111_TONECURVE_TABLE189_MASK                            0xfff0000
#define FCURVE_111_TONECURVE_TABLE189_LSB                                    16

/* ISP_FCURVE_112 */
#define FCURVE_112_TONECURVE_TABLE190_MASK                                0xfff
#define FCURVE_112_TONECURVE_TABLE190_LSB                                     0
#define FCURVE_112_TONECURVE_TABLE191_MASK                            0xfff0000
#define FCURVE_112_TONECURVE_TABLE191_LSB                                    16

/* ISP_FCURVE_113 */
#define FCURVE_113_TONECURVE_TABLE192_MASK                                0xfff
#define FCURVE_113_TONECURVE_TABLE192_LSB                                     0
#define FCURVE_113_TONECURVE_TABLE193_MASK                            0xfff0000
#define FCURVE_113_TONECURVE_TABLE193_LSB                                    16

/* ISP_FCURVE_114 */
#define FCURVE_114_TONECURVE_TABLE194_MASK                                0xfff
#define FCURVE_114_TONECURVE_TABLE194_LSB                                     0
#define FCURVE_114_TONECURVE_TABLE195_MASK                            0xfff0000
#define FCURVE_114_TONECURVE_TABLE195_LSB                                    16

/* ISP_FCURVE_115 */
#define FCURVE_115_TONECURVE_TABLE196_MASK                                0xfff
#define FCURVE_115_TONECURVE_TABLE196_LSB                                     0
#define FCURVE_115_TONECURVE_TABLE197_MASK                            0xfff0000
#define FCURVE_115_TONECURVE_TABLE197_LSB                                    16

/* ISP_FCURVE_116 */
#define FCURVE_116_TONECURVE_TABLE198_MASK                                0xfff
#define FCURVE_116_TONECURVE_TABLE198_LSB                                     0
#define FCURVE_116_TONECURVE_TABLE199_MASK                            0xfff0000
#define FCURVE_116_TONECURVE_TABLE199_LSB                                    16

/* ISP_FCURVE_117 */
#define FCURVE_117_TONECURVE_TABLE200_MASK                                0xfff
#define FCURVE_117_TONECURVE_TABLE200_LSB                                     0
#define FCURVE_117_TONECURVE_TABLE201_MASK                            0xfff0000
#define FCURVE_117_TONECURVE_TABLE201_LSB                                    16

/* ISP_FCURVE_118 */
#define FCURVE_118_TONECURVE_TABLE202_MASK                                0xfff
#define FCURVE_118_TONECURVE_TABLE202_LSB                                     0
#define FCURVE_118_TONECURVE_TABLE203_MASK                            0xfff0000
#define FCURVE_118_TONECURVE_TABLE203_LSB                                    16

/* ISP_FCURVE_119 */
#define FCURVE_119_TONECURVE_TABLE204_MASK                                0xfff
#define FCURVE_119_TONECURVE_TABLE204_LSB                                     0
#define FCURVE_119_TONECURVE_TABLE205_MASK                            0xfff0000
#define FCURVE_119_TONECURVE_TABLE205_LSB                                    16

/* ISP_FCURVE_120 */
#define FCURVE_120_TONECURVE_TABLE206_MASK                                0xfff
#define FCURVE_120_TONECURVE_TABLE206_LSB                                     0
#define FCURVE_120_TONECURVE_TABLE207_MASK                            0xfff0000
#define FCURVE_120_TONECURVE_TABLE207_LSB                                    16

/* ISP_FCURVE_121 */
#define FCURVE_121_TONECURVE_TABLE208_MASK                                0xfff
#define FCURVE_121_TONECURVE_TABLE208_LSB                                     0
#define FCURVE_121_TONECURVE_TABLE209_MASK                            0xfff0000
#define FCURVE_121_TONECURVE_TABLE209_LSB                                    16

/* ISP_FCURVE_122 */
#define FCURVE_122_TONECURVE_TABLE210_MASK                                0xfff
#define FCURVE_122_TONECURVE_TABLE210_LSB                                     0
#define FCURVE_122_TONECURVE_TABLE211_MASK                            0xfff0000
#define FCURVE_122_TONECURVE_TABLE211_LSB                                    16

/* ISP_FCURVE_123 */
#define FCURVE_123_TONECURVE_TABLE212_MASK                                0xfff
#define FCURVE_123_TONECURVE_TABLE212_LSB                                     0
#define FCURVE_123_TONECURVE_TABLE213_MASK                            0xfff0000
#define FCURVE_123_TONECURVE_TABLE213_LSB                                    16

/* ISP_FCURVE_124 */
#define FCURVE_124_TONECURVE_TABLE214_MASK                                0xfff
#define FCURVE_124_TONECURVE_TABLE214_LSB                                     0
#define FCURVE_124_TONECURVE_TABLE215_MASK                            0xfff0000
#define FCURVE_124_TONECURVE_TABLE215_LSB                                    16

/* ISP_FCURVE_125 */
#define FCURVE_125_TONECURVE_TABLE216_MASK                                0xfff
#define FCURVE_125_TONECURVE_TABLE216_LSB                                     0
#define FCURVE_125_TONECURVE_TABLE217_MASK                            0xfff0000
#define FCURVE_125_TONECURVE_TABLE217_LSB                                    16

/* ISP_FCURVE_126 */
#define FCURVE_126_TONECURVE_TABLE218_MASK                                0xfff
#define FCURVE_126_TONECURVE_TABLE218_LSB                                     0
#define FCURVE_126_TONECURVE_TABLE219_MASK                            0xfff0000
#define FCURVE_126_TONECURVE_TABLE219_LSB                                    16

/* ISP_FCURVE_127 */
#define FCURVE_127_TONECURVE_TABLE220_MASK                                0xfff
#define FCURVE_127_TONECURVE_TABLE220_LSB                                     0
#define FCURVE_127_TONECURVE_TABLE221_MASK                            0xfff0000
#define FCURVE_127_TONECURVE_TABLE221_LSB                                    16

/* ISP_FCURVE_128 */
#define FCURVE_128_TONECURVE_TABLE222_MASK                                0xfff
#define FCURVE_128_TONECURVE_TABLE222_LSB                                     0
#define FCURVE_128_TONECURVE_TABLE223_MASK                            0xfff0000
#define FCURVE_128_TONECURVE_TABLE223_LSB                                    16

/* ISP_FCURVE_129 */
#define FCURVE_129_TONECURVE_TABLE224_MASK                                0xfff
#define FCURVE_129_TONECURVE_TABLE224_LSB                                     0
#define FCURVE_129_TONECURVE_TABLE225_MASK                            0xfff0000
#define FCURVE_129_TONECURVE_TABLE225_LSB                                    16

/* ISP_FCURVE_130 */
#define FCURVE_130_TONECURVE_TABLE226_MASK                                0xfff
#define FCURVE_130_TONECURVE_TABLE226_LSB                                     0
#define FCURVE_130_TONECURVE_TABLE227_MASK                            0xfff0000
#define FCURVE_130_TONECURVE_TABLE227_LSB                                    16

/* ISP_FCURVE_131 */
#define FCURVE_131_TONECURVE_TABLE228_MASK                                0xfff
#define FCURVE_131_TONECURVE_TABLE228_LSB                                     0
#define FCURVE_131_TONECURVE_TABLE229_MASK                            0xfff0000
#define FCURVE_131_TONECURVE_TABLE229_LSB                                    16

/* ISP_FCURVE_132 */
#define FCURVE_132_TONECURVE_TABLE230_MASK                                0xfff
#define FCURVE_132_TONECURVE_TABLE230_LSB                                     0
#define FCURVE_132_TONECURVE_TABLE231_MASK                            0xfff0000
#define FCURVE_132_TONECURVE_TABLE231_LSB                                    16

/* ISP_FCURVE_133 */
#define FCURVE_133_TONECURVE_TABLE232_MASK                                0xfff
#define FCURVE_133_TONECURVE_TABLE232_LSB                                     0
#define FCURVE_133_TONECURVE_TABLE233_MASK                            0xfff0000
#define FCURVE_133_TONECURVE_TABLE233_LSB                                    16

/* ISP_FCURVE_134 */
#define FCURVE_134_TONECURVE_TABLE234_MASK                                0xfff
#define FCURVE_134_TONECURVE_TABLE234_LSB                                     0
#define FCURVE_134_TONECURVE_TABLE235_MASK                            0xfff0000
#define FCURVE_134_TONECURVE_TABLE235_LSB                                    16

/* ISP_FCURVE_135 */
#define FCURVE_135_TONECURVE_TABLE236_MASK                                0xfff
#define FCURVE_135_TONECURVE_TABLE236_LSB                                     0
#define FCURVE_135_TONECURVE_TABLE237_MASK                            0xfff0000
#define FCURVE_135_TONECURVE_TABLE237_LSB                                    16

/* ISP_FCURVE_136 */
#define FCURVE_136_TONECURVE_TABLE238_MASK                                0xfff
#define FCURVE_136_TONECURVE_TABLE238_LSB                                     0
#define FCURVE_136_TONECURVE_TABLE239_MASK                            0xfff0000
#define FCURVE_136_TONECURVE_TABLE239_LSB                                    16

/* ISP_FCURVE_137 */
#define FCURVE_137_TONECURVE_TABLE240_MASK                                0xfff
#define FCURVE_137_TONECURVE_TABLE240_LSB                                     0
#define FCURVE_137_TONECURVE_TABLE241_MASK                            0xfff0000
#define FCURVE_137_TONECURVE_TABLE241_LSB                                    16

/* ISP_FCURVE_138 */
#define FCURVE_138_TONECURVE_TABLE242_MASK                                0xfff
#define FCURVE_138_TONECURVE_TABLE242_LSB                                     0
#define FCURVE_138_TONECURVE_TABLE243_MASK                            0xfff0000
#define FCURVE_138_TONECURVE_TABLE243_LSB                                    16

/* ISP_FCURVE_139 */
#define FCURVE_139_TONECURVE_TABLE244_MASK                                0xfff
#define FCURVE_139_TONECURVE_TABLE244_LSB                                     0
#define FCURVE_139_TONECURVE_TABLE245_MASK                            0xfff0000
#define FCURVE_139_TONECURVE_TABLE245_LSB                                    16

/* ISP_FCURVE_140 */
#define FCURVE_140_TONECURVE_TABLE246_MASK                                0xfff
#define FCURVE_140_TONECURVE_TABLE246_LSB                                     0
#define FCURVE_140_TONECURVE_TABLE247_MASK                            0xfff0000
#define FCURVE_140_TONECURVE_TABLE247_LSB                                    16

/* ISP_FCURVE_141 */
#define FCURVE_141_TONECURVE_TABLE248_MASK                                0xfff
#define FCURVE_141_TONECURVE_TABLE248_LSB                                     0
#define FCURVE_141_TONECURVE_TABLE249_MASK                            0xfff0000
#define FCURVE_141_TONECURVE_TABLE249_LSB                                    16

/* ISP_FCURVE_142 */
#define FCURVE_142_TONECURVE_TABLE250_MASK                                0xfff
#define FCURVE_142_TONECURVE_TABLE250_LSB                                     0
#define FCURVE_142_TONECURVE_TABLE251_MASK                            0xfff0000
#define FCURVE_142_TONECURVE_TABLE251_LSB                                    16

/* ISP_FCURVE_143 */
#define FCURVE_143_TONECURVE_TABLE252_MASK                                0xfff
#define FCURVE_143_TONECURVE_TABLE252_LSB                                     0
#define FCURVE_143_TONECURVE_TABLE253_MASK                            0xfff0000
#define FCURVE_143_TONECURVE_TABLE253_LSB                                    16

/* ISP_FCURVE_144 */
#define FCURVE_144_TONECURVE_TABLE254_MASK                                0xfff
#define FCURVE_144_TONECURVE_TABLE254_LSB                                     0
#define FCURVE_144_TONECURVE_TABLE255_MASK                            0xfff0000
#define FCURVE_144_TONECURVE_TABLE255_LSB                                    16

/* ISP_FCURVE_145 */
#define FCURVE_145_TONECURVE_TABLE256_MASK                                0xfff
#define FCURVE_145_TONECURVE_TABLE256_LSB                                     0

/* ISP_FCURVE_146 */
#define FCURVE_146_TONECURVE_BRIGHT_TABLE0_MASK                           0xfff
#define FCURVE_146_TONECURVE_BRIGHT_TABLE0_LSB                                0
#define FCURVE_146_TONECURVE_BRIGHT_TABLE1_MASK                       0xfff0000
#define FCURVE_146_TONECURVE_BRIGHT_TABLE1_LSB                               16

/* ISP_FCURVE_147 */
#define FCURVE_147_TONECURVE_BRIGHT_TABLE2_MASK                           0xfff
#define FCURVE_147_TONECURVE_BRIGHT_TABLE2_LSB                                0
#define FCURVE_147_TONECURVE_BRIGHT_TABLE3_MASK                       0xfff0000
#define FCURVE_147_TONECURVE_BRIGHT_TABLE3_LSB                               16

/* ISP_FCURVE_148 */
#define FCURVE_148_TONECURVE_BRIGHT_TABLE4_MASK                           0xfff
#define FCURVE_148_TONECURVE_BRIGHT_TABLE4_LSB                                0
#define FCURVE_148_TONECURVE_BRIGHT_TABLE5_MASK                       0xfff0000
#define FCURVE_148_TONECURVE_BRIGHT_TABLE5_LSB                               16

/* ISP_FCURVE_149 */
#define FCURVE_149_TONECURVE_BRIGHT_TABLE6_MASK                           0xfff
#define FCURVE_149_TONECURVE_BRIGHT_TABLE6_LSB                                0
#define FCURVE_149_TONECURVE_BRIGHT_TABLE7_MASK                       0xfff0000
#define FCURVE_149_TONECURVE_BRIGHT_TABLE7_LSB                               16

/* ISP_FCURVE_150 */
#define FCURVE_150_TONECURVE_BRIGHT_TABLE8_MASK                           0xfff
#define FCURVE_150_TONECURVE_BRIGHT_TABLE8_LSB                                0
#define FCURVE_150_TONECURVE_BRIGHT_TABLE9_MASK                       0xfff0000
#define FCURVE_150_TONECURVE_BRIGHT_TABLE9_LSB                               16

/* ISP_FCURVE_151 */
#define FCURVE_151_TONECURVE_BRIGHT_TABLE10_MASK                          0xfff
#define FCURVE_151_TONECURVE_BRIGHT_TABLE10_LSB                               0
#define FCURVE_151_TONECURVE_BRIGHT_TABLE11_MASK                      0xfff0000
#define FCURVE_151_TONECURVE_BRIGHT_TABLE11_LSB                              16

/* ISP_FCURVE_152 */
#define FCURVE_152_TONECURVE_BRIGHT_TABLE12_MASK                          0xfff
#define FCURVE_152_TONECURVE_BRIGHT_TABLE12_LSB                               0
#define FCURVE_152_TONECURVE_BRIGHT_TABLE13_MASK                      0xfff0000
#define FCURVE_152_TONECURVE_BRIGHT_TABLE13_LSB                              16

/* ISP_FCURVE_153 */
#define FCURVE_153_TONECURVE_BRIGHT_TABLE14_MASK                          0xfff
#define FCURVE_153_TONECURVE_BRIGHT_TABLE14_LSB                               0
#define FCURVE_153_TONECURVE_BRIGHT_TABLE15_MASK                      0xfff0000
#define FCURVE_153_TONECURVE_BRIGHT_TABLE15_LSB                              16

/* ISP_FCURVE_154 */
#define FCURVE_154_TONECURVE_BRIGHT_TABLE16_MASK                          0xfff
#define FCURVE_154_TONECURVE_BRIGHT_TABLE16_LSB                               0
#define FCURVE_154_TONECURVE_BRIGHT_TABLE17_MASK                      0xfff0000
#define FCURVE_154_TONECURVE_BRIGHT_TABLE17_LSB                              16

/* ISP_FCURVE_155 */
#define FCURVE_155_TONECURVE_BRIGHT_TABLE18_MASK                          0xfff
#define FCURVE_155_TONECURVE_BRIGHT_TABLE18_LSB                               0
#define FCURVE_155_TONECURVE_BRIGHT_TABLE19_MASK                      0xfff0000
#define FCURVE_155_TONECURVE_BRIGHT_TABLE19_LSB                              16

/* ISP_FCURVE_156 */
#define FCURVE_156_TONECURVE_BRIGHT_TABLE20_MASK                          0xfff
#define FCURVE_156_TONECURVE_BRIGHT_TABLE20_LSB                               0
#define FCURVE_156_TONECURVE_BRIGHT_TABLE21_MASK                      0xfff0000
#define FCURVE_156_TONECURVE_BRIGHT_TABLE21_LSB                              16

/* ISP_FCURVE_157 */
#define FCURVE_157_TONECURVE_BRIGHT_TABLE22_MASK                          0xfff
#define FCURVE_157_TONECURVE_BRIGHT_TABLE22_LSB                               0
#define FCURVE_157_TONECURVE_BRIGHT_TABLE23_MASK                      0xfff0000
#define FCURVE_157_TONECURVE_BRIGHT_TABLE23_LSB                              16

/* ISP_FCURVE_158 */
#define FCURVE_158_TONECURVE_BRIGHT_TABLE24_MASK                          0xfff
#define FCURVE_158_TONECURVE_BRIGHT_TABLE24_LSB                               0
#define FCURVE_158_TONECURVE_BRIGHT_TABLE25_MASK                      0xfff0000
#define FCURVE_158_TONECURVE_BRIGHT_TABLE25_LSB                              16

/* ISP_FCURVE_159 */
#define FCURVE_159_TONECURVE_BRIGHT_TABLE26_MASK                          0xfff
#define FCURVE_159_TONECURVE_BRIGHT_TABLE26_LSB                               0
#define FCURVE_159_TONECURVE_BRIGHT_TABLE27_MASK                      0xfff0000
#define FCURVE_159_TONECURVE_BRIGHT_TABLE27_LSB                              16

/* ISP_FCURVE_160 */
#define FCURVE_160_TONECURVE_BRIGHT_TABLE28_MASK                          0xfff
#define FCURVE_160_TONECURVE_BRIGHT_TABLE28_LSB                               0
#define FCURVE_160_TONECURVE_BRIGHT_TABLE29_MASK                      0xfff0000
#define FCURVE_160_TONECURVE_BRIGHT_TABLE29_LSB                              16

/* ISP_FCURVE_161 */
#define FCURVE_161_TONECURVE_BRIGHT_TABLE30_MASK                          0xfff
#define FCURVE_161_TONECURVE_BRIGHT_TABLE30_LSB                               0
#define FCURVE_161_TONECURVE_BRIGHT_TABLE31_MASK                      0xfff0000
#define FCURVE_161_TONECURVE_BRIGHT_TABLE31_LSB                              16

/* ISP_FCURVE_162 */
#define FCURVE_162_TONECURVE_BRIGHT_TABLE32_MASK                          0xfff
#define FCURVE_162_TONECURVE_BRIGHT_TABLE32_LSB                               0
#define FCURVE_162_TONECURVE_BRIGHT_TABLE33_MASK                      0xfff0000
#define FCURVE_162_TONECURVE_BRIGHT_TABLE33_LSB                              16

/* ISP_FCURVE_163 */
#define FCURVE_163_TONECURVE_BRIGHT_TABLE34_MASK                          0xfff
#define FCURVE_163_TONECURVE_BRIGHT_TABLE34_LSB                               0
#define FCURVE_163_TONECURVE_BRIGHT_TABLE35_MASK                      0xfff0000
#define FCURVE_163_TONECURVE_BRIGHT_TABLE35_LSB                              16

/* ISP_FCURVE_164 */
#define FCURVE_164_TONECURVE_BRIGHT_TABLE36_MASK                          0xfff
#define FCURVE_164_TONECURVE_BRIGHT_TABLE36_LSB                               0
#define FCURVE_164_TONECURVE_BRIGHT_TABLE37_MASK                      0xfff0000
#define FCURVE_164_TONECURVE_BRIGHT_TABLE37_LSB                              16

/* ISP_FCURVE_165 */
#define FCURVE_165_TONECURVE_BRIGHT_TABLE38_MASK                          0xfff
#define FCURVE_165_TONECURVE_BRIGHT_TABLE38_LSB                               0
#define FCURVE_165_TONECURVE_BRIGHT_TABLE39_MASK                      0xfff0000
#define FCURVE_165_TONECURVE_BRIGHT_TABLE39_LSB                              16

/* ISP_FCURVE_166 */
#define FCURVE_166_TONECURVE_BRIGHT_TABLE40_MASK                          0xfff
#define FCURVE_166_TONECURVE_BRIGHT_TABLE40_LSB                               0
#define FCURVE_166_TONECURVE_BRIGHT_TABLE41_MASK                      0xfff0000
#define FCURVE_166_TONECURVE_BRIGHT_TABLE41_LSB                              16

/* ISP_FCURVE_167 */
#define FCURVE_167_TONECURVE_BRIGHT_TABLE42_MASK                          0xfff
#define FCURVE_167_TONECURVE_BRIGHT_TABLE42_LSB                               0
#define FCURVE_167_TONECURVE_BRIGHT_TABLE43_MASK                      0xfff0000
#define FCURVE_167_TONECURVE_BRIGHT_TABLE43_LSB                              16

/* ISP_FCURVE_168 */
#define FCURVE_168_TONECURVE_BRIGHT_TABLE44_MASK                          0xfff
#define FCURVE_168_TONECURVE_BRIGHT_TABLE44_LSB                               0
#define FCURVE_168_TONECURVE_BRIGHT_TABLE45_MASK                      0xfff0000
#define FCURVE_168_TONECURVE_BRIGHT_TABLE45_LSB                              16

/* ISP_FCURVE_169 */
#define FCURVE_169_TONECURVE_BRIGHT_TABLE46_MASK                          0xfff
#define FCURVE_169_TONECURVE_BRIGHT_TABLE46_LSB                               0
#define FCURVE_169_TONECURVE_BRIGHT_TABLE47_MASK                      0xfff0000
#define FCURVE_169_TONECURVE_BRIGHT_TABLE47_LSB                              16

/* ISP_FCURVE_170 */
#define FCURVE_170_TONECURVE_BRIGHT_TABLE48_MASK                          0xfff
#define FCURVE_170_TONECURVE_BRIGHT_TABLE48_LSB                               0
#define FCURVE_170_TONECURVE_BRIGHT_TABLE49_MASK                      0xfff0000
#define FCURVE_170_TONECURVE_BRIGHT_TABLE49_LSB                              16

/* ISP_FCURVE_171 */
#define FCURVE_171_TONECURVE_BRIGHT_TABLE50_MASK                          0xfff
#define FCURVE_171_TONECURVE_BRIGHT_TABLE50_LSB                               0
#define FCURVE_171_TONECURVE_BRIGHT_TABLE51_MASK                      0xfff0000
#define FCURVE_171_TONECURVE_BRIGHT_TABLE51_LSB                              16

/* ISP_FCURVE_172 */
#define FCURVE_172_TONECURVE_BRIGHT_TABLE52_MASK                          0xfff
#define FCURVE_172_TONECURVE_BRIGHT_TABLE52_LSB                               0
#define FCURVE_172_TONECURVE_BRIGHT_TABLE53_MASK                      0xfff0000
#define FCURVE_172_TONECURVE_BRIGHT_TABLE53_LSB                              16

/* ISP_FCURVE_173 */
#define FCURVE_173_TONECURVE_BRIGHT_TABLE54_MASK                          0xfff
#define FCURVE_173_TONECURVE_BRIGHT_TABLE54_LSB                               0
#define FCURVE_173_TONECURVE_BRIGHT_TABLE55_MASK                      0xfff0000
#define FCURVE_173_TONECURVE_BRIGHT_TABLE55_LSB                              16

/* ISP_FCURVE_174 */
#define FCURVE_174_TONECURVE_BRIGHT_TABLE56_MASK                          0xfff
#define FCURVE_174_TONECURVE_BRIGHT_TABLE56_LSB                               0
#define FCURVE_174_TONECURVE_BRIGHT_TABLE57_MASK                      0xfff0000
#define FCURVE_174_TONECURVE_BRIGHT_TABLE57_LSB                              16

/* ISP_FCURVE_175 */
#define FCURVE_175_TONECURVE_BRIGHT_TABLE58_MASK                          0xfff
#define FCURVE_175_TONECURVE_BRIGHT_TABLE58_LSB                               0
#define FCURVE_175_TONECURVE_BRIGHT_TABLE59_MASK                      0xfff0000
#define FCURVE_175_TONECURVE_BRIGHT_TABLE59_LSB                              16

/* ISP_FCURVE_176 */
#define FCURVE_176_TONECURVE_BRIGHT_TABLE60_MASK                          0xfff
#define FCURVE_176_TONECURVE_BRIGHT_TABLE60_LSB                               0
#define FCURVE_176_TONECURVE_BRIGHT_TABLE61_MASK                      0xfff0000
#define FCURVE_176_TONECURVE_BRIGHT_TABLE61_LSB                              16

/* ISP_FCURVE_177 */
#define FCURVE_177_TONECURVE_BRIGHT_TABLE62_MASK                          0xfff
#define FCURVE_177_TONECURVE_BRIGHT_TABLE62_LSB                               0
#define FCURVE_177_TONECURVE_BRIGHT_TABLE63_MASK                      0xfff0000
#define FCURVE_177_TONECURVE_BRIGHT_TABLE63_LSB                              16

/* ISP_FCURVE_178 */
#define FCURVE_178_TONECURVE_BRIGHT_TABLE64_MASK                          0xfff
#define FCURVE_178_TONECURVE_BRIGHT_TABLE64_LSB                               0
#define FCURVE_178_TONECURVE_BRIGHT_TABLE65_MASK                      0xfff0000
#define FCURVE_178_TONECURVE_BRIGHT_TABLE65_LSB                              16

/* ISP_FCURVE_179 */
#define FCURVE_179_TONECURVE_BRIGHT_TABLE66_MASK                          0xfff
#define FCURVE_179_TONECURVE_BRIGHT_TABLE66_LSB                               0
#define FCURVE_179_TONECURVE_BRIGHT_TABLE67_MASK                      0xfff0000
#define FCURVE_179_TONECURVE_BRIGHT_TABLE67_LSB                              16

/* ISP_FCURVE_180 */
#define FCURVE_180_TONECURVE_BRIGHT_TABLE68_MASK                          0xfff
#define FCURVE_180_TONECURVE_BRIGHT_TABLE68_LSB                               0
#define FCURVE_180_TONECURVE_BRIGHT_TABLE69_MASK                      0xfff0000
#define FCURVE_180_TONECURVE_BRIGHT_TABLE69_LSB                              16

/* ISP_FCURVE_181 */
#define FCURVE_181_TONECURVE_BRIGHT_TABLE70_MASK                          0xfff
#define FCURVE_181_TONECURVE_BRIGHT_TABLE70_LSB                               0
#define FCURVE_181_TONECURVE_BRIGHT_TABLE71_MASK                      0xfff0000
#define FCURVE_181_TONECURVE_BRIGHT_TABLE71_LSB                              16

/* ISP_FCURVE_182 */
#define FCURVE_182_TONECURVE_BRIGHT_TABLE72_MASK                          0xfff
#define FCURVE_182_TONECURVE_BRIGHT_TABLE72_LSB                               0
#define FCURVE_182_TONECURVE_BRIGHT_TABLE73_MASK                      0xfff0000
#define FCURVE_182_TONECURVE_BRIGHT_TABLE73_LSB                              16

/* ISP_FCURVE_183 */
#define FCURVE_183_TONECURVE_BRIGHT_TABLE74_MASK                          0xfff
#define FCURVE_183_TONECURVE_BRIGHT_TABLE74_LSB                               0
#define FCURVE_183_TONECURVE_BRIGHT_TABLE75_MASK                      0xfff0000
#define FCURVE_183_TONECURVE_BRIGHT_TABLE75_LSB                              16

/* ISP_FCURVE_184 */
#define FCURVE_184_TONECURVE_BRIGHT_TABLE76_MASK                          0xfff
#define FCURVE_184_TONECURVE_BRIGHT_TABLE76_LSB                               0
#define FCURVE_184_TONECURVE_BRIGHT_TABLE77_MASK                      0xfff0000
#define FCURVE_184_TONECURVE_BRIGHT_TABLE77_LSB                              16

/* ISP_FCURVE_185 */
#define FCURVE_185_TONECURVE_BRIGHT_TABLE78_MASK                          0xfff
#define FCURVE_185_TONECURVE_BRIGHT_TABLE78_LSB                               0
#define FCURVE_185_TONECURVE_BRIGHT_TABLE79_MASK                      0xfff0000
#define FCURVE_185_TONECURVE_BRIGHT_TABLE79_LSB                              16

/* ISP_FCURVE_186 */
#define FCURVE_186_TONECURVE_BRIGHT_TABLE80_MASK                          0xfff
#define FCURVE_186_TONECURVE_BRIGHT_TABLE80_LSB                               0
#define FCURVE_186_TONECURVE_BRIGHT_TABLE81_MASK                      0xfff0000
#define FCURVE_186_TONECURVE_BRIGHT_TABLE81_LSB                              16

/* ISP_FCURVE_187 */
#define FCURVE_187_TONECURVE_BRIGHT_TABLE82_MASK                          0xfff
#define FCURVE_187_TONECURVE_BRIGHT_TABLE82_LSB                               0
#define FCURVE_187_TONECURVE_BRIGHT_TABLE83_MASK                      0xfff0000
#define FCURVE_187_TONECURVE_BRIGHT_TABLE83_LSB                              16

/* ISP_FCURVE_188 */
#define FCURVE_188_TONECURVE_BRIGHT_TABLE84_MASK                          0xfff
#define FCURVE_188_TONECURVE_BRIGHT_TABLE84_LSB                               0
#define FCURVE_188_TONECURVE_BRIGHT_TABLE85_MASK                      0xfff0000
#define FCURVE_188_TONECURVE_BRIGHT_TABLE85_LSB                              16

/* ISP_FCURVE_189 */
#define FCURVE_189_TONECURVE_BRIGHT_TABLE86_MASK                          0xfff
#define FCURVE_189_TONECURVE_BRIGHT_TABLE86_LSB                               0
#define FCURVE_189_TONECURVE_BRIGHT_TABLE87_MASK                      0xfff0000
#define FCURVE_189_TONECURVE_BRIGHT_TABLE87_LSB                              16

/* ISP_FCURVE_190 */
#define FCURVE_190_TONECURVE_BRIGHT_TABLE88_MASK                          0xfff
#define FCURVE_190_TONECURVE_BRIGHT_TABLE88_LSB                               0
#define FCURVE_190_TONECURVE_BRIGHT_TABLE89_MASK                      0xfff0000
#define FCURVE_190_TONECURVE_BRIGHT_TABLE89_LSB                              16

/* ISP_FCURVE_191 */
#define FCURVE_191_TONECURVE_BRIGHT_TABLE90_MASK                          0xfff
#define FCURVE_191_TONECURVE_BRIGHT_TABLE90_LSB                               0
#define FCURVE_191_TONECURVE_BRIGHT_TABLE91_MASK                      0xfff0000
#define FCURVE_191_TONECURVE_BRIGHT_TABLE91_LSB                              16

/* ISP_FCURVE_192 */
#define FCURVE_192_TONECURVE_BRIGHT_TABLE92_MASK                          0xfff
#define FCURVE_192_TONECURVE_BRIGHT_TABLE92_LSB                               0
#define FCURVE_192_TONECURVE_BRIGHT_TABLE93_MASK                      0xfff0000
#define FCURVE_192_TONECURVE_BRIGHT_TABLE93_LSB                              16

/* ISP_FCURVE_193 */
#define FCURVE_193_TONECURVE_BRIGHT_TABLE94_MASK                          0xfff
#define FCURVE_193_TONECURVE_BRIGHT_TABLE94_LSB                               0
#define FCURVE_193_TONECURVE_BRIGHT_TABLE95_MASK                      0xfff0000
#define FCURVE_193_TONECURVE_BRIGHT_TABLE95_LSB                              16

/* ISP_FCURVE_194 */
#define FCURVE_194_TONECURVE_BRIGHT_TABLE96_MASK                          0xfff
#define FCURVE_194_TONECURVE_BRIGHT_TABLE96_LSB                               0
#define FCURVE_194_TONECURVE_BRIGHT_TABLE97_MASK                      0xfff0000
#define FCURVE_194_TONECURVE_BRIGHT_TABLE97_LSB                              16

/* ISP_FCURVE_195 */
#define FCURVE_195_TONECURVE_BRIGHT_TABLE98_MASK                          0xfff
#define FCURVE_195_TONECURVE_BRIGHT_TABLE98_LSB                               0
#define FCURVE_195_TONECURVE_BRIGHT_TABLE99_MASK                      0xfff0000
#define FCURVE_195_TONECURVE_BRIGHT_TABLE99_LSB                              16

/* ISP_FCURVE_196 */
#define FCURVE_196_TONECURVE_BRIGHT_TABLE100_MASK                         0xfff
#define FCURVE_196_TONECURVE_BRIGHT_TABLE100_LSB                              0
#define FCURVE_196_TONECURVE_BRIGHT_TABLE101_MASK                     0xfff0000
#define FCURVE_196_TONECURVE_BRIGHT_TABLE101_LSB                             16

/* ISP_FCURVE_197 */
#define FCURVE_197_TONECURVE_BRIGHT_TABLE102_MASK                         0xfff
#define FCURVE_197_TONECURVE_BRIGHT_TABLE102_LSB                              0
#define FCURVE_197_TONECURVE_BRIGHT_TABLE103_MASK                     0xfff0000
#define FCURVE_197_TONECURVE_BRIGHT_TABLE103_LSB                             16

/* ISP_FCURVE_198 */
#define FCURVE_198_TONECURVE_BRIGHT_TABLE104_MASK                         0xfff
#define FCURVE_198_TONECURVE_BRIGHT_TABLE104_LSB                              0
#define FCURVE_198_TONECURVE_BRIGHT_TABLE105_MASK                     0xfff0000
#define FCURVE_198_TONECURVE_BRIGHT_TABLE105_LSB                             16

/* ISP_FCURVE_199 */
#define FCURVE_199_TONECURVE_BRIGHT_TABLE106_MASK                         0xfff
#define FCURVE_199_TONECURVE_BRIGHT_TABLE106_LSB                              0
#define FCURVE_199_TONECURVE_BRIGHT_TABLE107_MASK                     0xfff0000
#define FCURVE_199_TONECURVE_BRIGHT_TABLE107_LSB                             16

/* ISP_FCURVE_200 */
#define FCURVE_200_TONECURVE_BRIGHT_TABLE108_MASK                         0xfff
#define FCURVE_200_TONECURVE_BRIGHT_TABLE108_LSB                              0
#define FCURVE_200_TONECURVE_BRIGHT_TABLE109_MASK                     0xfff0000
#define FCURVE_200_TONECURVE_BRIGHT_TABLE109_LSB                             16

/* ISP_FCURVE_201 */
#define FCURVE_201_TONECURVE_BRIGHT_TABLE110_MASK                         0xfff
#define FCURVE_201_TONECURVE_BRIGHT_TABLE110_LSB                              0
#define FCURVE_201_TONECURVE_BRIGHT_TABLE111_MASK                     0xfff0000
#define FCURVE_201_TONECURVE_BRIGHT_TABLE111_LSB                             16

/* ISP_FCURVE_202 */
#define FCURVE_202_TONECURVE_BRIGHT_TABLE112_MASK                         0xfff
#define FCURVE_202_TONECURVE_BRIGHT_TABLE112_LSB                              0
#define FCURVE_202_TONECURVE_BRIGHT_TABLE113_MASK                     0xfff0000
#define FCURVE_202_TONECURVE_BRIGHT_TABLE113_LSB                             16

/* ISP_FCURVE_203 */
#define FCURVE_203_TONECURVE_BRIGHT_TABLE114_MASK                         0xfff
#define FCURVE_203_TONECURVE_BRIGHT_TABLE114_LSB                              0
#define FCURVE_203_TONECURVE_BRIGHT_TABLE115_MASK                     0xfff0000
#define FCURVE_203_TONECURVE_BRIGHT_TABLE115_LSB                             16

/* ISP_FCURVE_204 */
#define FCURVE_204_TONECURVE_BRIGHT_TABLE116_MASK                         0xfff
#define FCURVE_204_TONECURVE_BRIGHT_TABLE116_LSB                              0
#define FCURVE_204_TONECURVE_BRIGHT_TABLE117_MASK                     0xfff0000
#define FCURVE_204_TONECURVE_BRIGHT_TABLE117_LSB                             16

/* ISP_FCURVE_205 */
#define FCURVE_205_TONECURVE_BRIGHT_TABLE118_MASK                         0xfff
#define FCURVE_205_TONECURVE_BRIGHT_TABLE118_LSB                              0
#define FCURVE_205_TONECURVE_BRIGHT_TABLE119_MASK                     0xfff0000
#define FCURVE_205_TONECURVE_BRIGHT_TABLE119_LSB                             16

/* ISP_FCURVE_206 */
#define FCURVE_206_TONECURVE_BRIGHT_TABLE120_MASK                         0xfff
#define FCURVE_206_TONECURVE_BRIGHT_TABLE120_LSB                              0
#define FCURVE_206_TONECURVE_BRIGHT_TABLE121_MASK                     0xfff0000
#define FCURVE_206_TONECURVE_BRIGHT_TABLE121_LSB                             16

/* ISP_FCURVE_207 */
#define FCURVE_207_TONECURVE_BRIGHT_TABLE122_MASK                         0xfff
#define FCURVE_207_TONECURVE_BRIGHT_TABLE122_LSB                              0
#define FCURVE_207_TONECURVE_BRIGHT_TABLE123_MASK                     0xfff0000
#define FCURVE_207_TONECURVE_BRIGHT_TABLE123_LSB                             16

/* ISP_FCURVE_208 */
#define FCURVE_208_TONECURVE_BRIGHT_TABLE124_MASK                         0xfff
#define FCURVE_208_TONECURVE_BRIGHT_TABLE124_LSB                              0
#define FCURVE_208_TONECURVE_BRIGHT_TABLE125_MASK                     0xfff0000
#define FCURVE_208_TONECURVE_BRIGHT_TABLE125_LSB                             16

/* ISP_FCURVE_209 */
#define FCURVE_209_TONECURVE_BRIGHT_TABLE126_MASK                         0xfff
#define FCURVE_209_TONECURVE_BRIGHT_TABLE126_LSB                              0
#define FCURVE_209_TONECURVE_BRIGHT_TABLE127_MASK                     0xfff0000
#define FCURVE_209_TONECURVE_BRIGHT_TABLE127_LSB                             16

/* ISP_FCURVE_210 */
#define FCURVE_210_TONECURVE_BRIGHT_TABLE128_MASK                         0xfff
#define FCURVE_210_TONECURVE_BRIGHT_TABLE128_LSB                              0
#define FCURVE_210_TONECURVE_BRIGHT_TABLE129_MASK                     0xfff0000
#define FCURVE_210_TONECURVE_BRIGHT_TABLE129_LSB                             16

/* ISP_FCURVE_211 */
#define FCURVE_211_TONECURVE_BRIGHT_TABLE130_MASK                         0xfff
#define FCURVE_211_TONECURVE_BRIGHT_TABLE130_LSB                              0
#define FCURVE_211_TONECURVE_BRIGHT_TABLE131_MASK                     0xfff0000
#define FCURVE_211_TONECURVE_BRIGHT_TABLE131_LSB                             16

/* ISP_FCURVE_212 */
#define FCURVE_212_TONECURVE_BRIGHT_TABLE132_MASK                         0xfff
#define FCURVE_212_TONECURVE_BRIGHT_TABLE132_LSB                              0
#define FCURVE_212_TONECURVE_BRIGHT_TABLE133_MASK                     0xfff0000
#define FCURVE_212_TONECURVE_BRIGHT_TABLE133_LSB                             16

/* ISP_FCURVE_213 */
#define FCURVE_213_TONECURVE_BRIGHT_TABLE134_MASK                         0xfff
#define FCURVE_213_TONECURVE_BRIGHT_TABLE134_LSB                              0
#define FCURVE_213_TONECURVE_BRIGHT_TABLE135_MASK                     0xfff0000
#define FCURVE_213_TONECURVE_BRIGHT_TABLE135_LSB                             16

/* ISP_FCURVE_214 */
#define FCURVE_214_TONECURVE_BRIGHT_TABLE136_MASK                         0xfff
#define FCURVE_214_TONECURVE_BRIGHT_TABLE136_LSB                              0
#define FCURVE_214_TONECURVE_BRIGHT_TABLE137_MASK                     0xfff0000
#define FCURVE_214_TONECURVE_BRIGHT_TABLE137_LSB                             16

/* ISP_FCURVE_215 */
#define FCURVE_215_TONECURVE_BRIGHT_TABLE138_MASK                         0xfff
#define FCURVE_215_TONECURVE_BRIGHT_TABLE138_LSB                              0
#define FCURVE_215_TONECURVE_BRIGHT_TABLE139_MASK                     0xfff0000
#define FCURVE_215_TONECURVE_BRIGHT_TABLE139_LSB                             16

/* ISP_FCURVE_216 */
#define FCURVE_216_TONECURVE_BRIGHT_TABLE140_MASK                         0xfff
#define FCURVE_216_TONECURVE_BRIGHT_TABLE140_LSB                              0
#define FCURVE_216_TONECURVE_BRIGHT_TABLE141_MASK                     0xfff0000
#define FCURVE_216_TONECURVE_BRIGHT_TABLE141_LSB                             16

/* ISP_FCURVE_217 */
#define FCURVE_217_TONECURVE_BRIGHT_TABLE142_MASK                         0xfff
#define FCURVE_217_TONECURVE_BRIGHT_TABLE142_LSB                              0
#define FCURVE_217_TONECURVE_BRIGHT_TABLE143_MASK                     0xfff0000
#define FCURVE_217_TONECURVE_BRIGHT_TABLE143_LSB                             16

/* ISP_FCURVE_218 */
#define FCURVE_218_TONECURVE_BRIGHT_TABLE144_MASK                         0xfff
#define FCURVE_218_TONECURVE_BRIGHT_TABLE144_LSB                              0
#define FCURVE_218_TONECURVE_BRIGHT_TABLE145_MASK                     0xfff0000
#define FCURVE_218_TONECURVE_BRIGHT_TABLE145_LSB                             16

/* ISP_FCURVE_219 */
#define FCURVE_219_TONECURVE_BRIGHT_TABLE146_MASK                         0xfff
#define FCURVE_219_TONECURVE_BRIGHT_TABLE146_LSB                              0
#define FCURVE_219_TONECURVE_BRIGHT_TABLE147_MASK                     0xfff0000
#define FCURVE_219_TONECURVE_BRIGHT_TABLE147_LSB                             16

/* ISP_FCURVE_220 */
#define FCURVE_220_TONECURVE_BRIGHT_TABLE148_MASK                         0xfff
#define FCURVE_220_TONECURVE_BRIGHT_TABLE148_LSB                              0
#define FCURVE_220_TONECURVE_BRIGHT_TABLE149_MASK                     0xfff0000
#define FCURVE_220_TONECURVE_BRIGHT_TABLE149_LSB                             16

/* ISP_FCURVE_221 */
#define FCURVE_221_TONECURVE_BRIGHT_TABLE150_MASK                         0xfff
#define FCURVE_221_TONECURVE_BRIGHT_TABLE150_LSB                              0
#define FCURVE_221_TONECURVE_BRIGHT_TABLE151_MASK                     0xfff0000
#define FCURVE_221_TONECURVE_BRIGHT_TABLE151_LSB                             16

/* ISP_FCURVE_222 */
#define FCURVE_222_TONECURVE_BRIGHT_TABLE152_MASK                         0xfff
#define FCURVE_222_TONECURVE_BRIGHT_TABLE152_LSB                              0
#define FCURVE_222_TONECURVE_BRIGHT_TABLE153_MASK                     0xfff0000
#define FCURVE_222_TONECURVE_BRIGHT_TABLE153_LSB                             16

/* ISP_FCURVE_223 */
#define FCURVE_223_TONECURVE_BRIGHT_TABLE154_MASK                         0xfff
#define FCURVE_223_TONECURVE_BRIGHT_TABLE154_LSB                              0
#define FCURVE_223_TONECURVE_BRIGHT_TABLE155_MASK                     0xfff0000
#define FCURVE_223_TONECURVE_BRIGHT_TABLE155_LSB                             16

/* ISP_FCURVE_224 */
#define FCURVE_224_TONECURVE_BRIGHT_TABLE156_MASK                         0xfff
#define FCURVE_224_TONECURVE_BRIGHT_TABLE156_LSB                              0
#define FCURVE_224_TONECURVE_BRIGHT_TABLE157_MASK                     0xfff0000
#define FCURVE_224_TONECURVE_BRIGHT_TABLE157_LSB                             16

/* ISP_FCURVE_225 */
#define FCURVE_225_TONECURVE_BRIGHT_TABLE158_MASK                         0xfff
#define FCURVE_225_TONECURVE_BRIGHT_TABLE158_LSB                              0
#define FCURVE_225_TONECURVE_BRIGHT_TABLE159_MASK                     0xfff0000
#define FCURVE_225_TONECURVE_BRIGHT_TABLE159_LSB                             16

/* ISP_FCURVE_226 */
#define FCURVE_226_TONECURVE_BRIGHT_TABLE160_MASK                         0xfff
#define FCURVE_226_TONECURVE_BRIGHT_TABLE160_LSB                              0
#define FCURVE_226_TONECURVE_BRIGHT_TABLE161_MASK                     0xfff0000
#define FCURVE_226_TONECURVE_BRIGHT_TABLE161_LSB                             16

/* ISP_FCURVE_227 */
#define FCURVE_227_TONECURVE_BRIGHT_TABLE162_MASK                         0xfff
#define FCURVE_227_TONECURVE_BRIGHT_TABLE162_LSB                              0
#define FCURVE_227_TONECURVE_BRIGHT_TABLE163_MASK                     0xfff0000
#define FCURVE_227_TONECURVE_BRIGHT_TABLE163_LSB                             16

/* ISP_FCURVE_228 */
#define FCURVE_228_TONECURVE_BRIGHT_TABLE164_MASK                         0xfff
#define FCURVE_228_TONECURVE_BRIGHT_TABLE164_LSB                              0
#define FCURVE_228_TONECURVE_BRIGHT_TABLE165_MASK                     0xfff0000
#define FCURVE_228_TONECURVE_BRIGHT_TABLE165_LSB                             16

/* ISP_FCURVE_229 */
#define FCURVE_229_TONECURVE_BRIGHT_TABLE166_MASK                         0xfff
#define FCURVE_229_TONECURVE_BRIGHT_TABLE166_LSB                              0
#define FCURVE_229_TONECURVE_BRIGHT_TABLE167_MASK                     0xfff0000
#define FCURVE_229_TONECURVE_BRIGHT_TABLE167_LSB                             16

/* ISP_FCURVE_230 */
#define FCURVE_230_TONECURVE_BRIGHT_TABLE168_MASK                         0xfff
#define FCURVE_230_TONECURVE_BRIGHT_TABLE168_LSB                              0
#define FCURVE_230_TONECURVE_BRIGHT_TABLE169_MASK                     0xfff0000
#define FCURVE_230_TONECURVE_BRIGHT_TABLE169_LSB                             16

/* ISP_FCURVE_231 */
#define FCURVE_231_TONECURVE_BRIGHT_TABLE170_MASK                         0xfff
#define FCURVE_231_TONECURVE_BRIGHT_TABLE170_LSB                              0
#define FCURVE_231_TONECURVE_BRIGHT_TABLE171_MASK                     0xfff0000
#define FCURVE_231_TONECURVE_BRIGHT_TABLE171_LSB                             16

/* ISP_FCURVE_232 */
#define FCURVE_232_TONECURVE_BRIGHT_TABLE172_MASK                         0xfff
#define FCURVE_232_TONECURVE_BRIGHT_TABLE172_LSB                              0
#define FCURVE_232_TONECURVE_BRIGHT_TABLE173_MASK                     0xfff0000
#define FCURVE_232_TONECURVE_BRIGHT_TABLE173_LSB                             16

/* ISP_FCURVE_233 */
#define FCURVE_233_TONECURVE_BRIGHT_TABLE174_MASK                         0xfff
#define FCURVE_233_TONECURVE_BRIGHT_TABLE174_LSB                              0
#define FCURVE_233_TONECURVE_BRIGHT_TABLE175_MASK                     0xfff0000
#define FCURVE_233_TONECURVE_BRIGHT_TABLE175_LSB                             16

/* ISP_FCURVE_234 */
#define FCURVE_234_TONECURVE_BRIGHT_TABLE176_MASK                         0xfff
#define FCURVE_234_TONECURVE_BRIGHT_TABLE176_LSB                              0
#define FCURVE_234_TONECURVE_BRIGHT_TABLE177_MASK                     0xfff0000
#define FCURVE_234_TONECURVE_BRIGHT_TABLE177_LSB                             16

/* ISP_FCURVE_235 */
#define FCURVE_235_TONECURVE_BRIGHT_TABLE178_MASK                         0xfff
#define FCURVE_235_TONECURVE_BRIGHT_TABLE178_LSB                              0
#define FCURVE_235_TONECURVE_BRIGHT_TABLE179_MASK                     0xfff0000
#define FCURVE_235_TONECURVE_BRIGHT_TABLE179_LSB                             16

/* ISP_FCURVE_236 */
#define FCURVE_236_TONECURVE_BRIGHT_TABLE180_MASK                         0xfff
#define FCURVE_236_TONECURVE_BRIGHT_TABLE180_LSB                              0
#define FCURVE_236_TONECURVE_BRIGHT_TABLE181_MASK                     0xfff0000
#define FCURVE_236_TONECURVE_BRIGHT_TABLE181_LSB                             16

/* ISP_FCURVE_237 */
#define FCURVE_237_TONECURVE_BRIGHT_TABLE182_MASK                         0xfff
#define FCURVE_237_TONECURVE_BRIGHT_TABLE182_LSB                              0
#define FCURVE_237_TONECURVE_BRIGHT_TABLE183_MASK                     0xfff0000
#define FCURVE_237_TONECURVE_BRIGHT_TABLE183_LSB                             16

/* ISP_FCURVE_238 */
#define FCURVE_238_TONECURVE_BRIGHT_TABLE184_MASK                         0xfff
#define FCURVE_238_TONECURVE_BRIGHT_TABLE184_LSB                              0
#define FCURVE_238_TONECURVE_BRIGHT_TABLE185_MASK                     0xfff0000
#define FCURVE_238_TONECURVE_BRIGHT_TABLE185_LSB                             16

/* ISP_FCURVE_239 */
#define FCURVE_239_TONECURVE_BRIGHT_TABLE186_MASK                         0xfff
#define FCURVE_239_TONECURVE_BRIGHT_TABLE186_LSB                              0
#define FCURVE_239_TONECURVE_BRIGHT_TABLE187_MASK                     0xfff0000
#define FCURVE_239_TONECURVE_BRIGHT_TABLE187_LSB                             16

/* ISP_FCURVE_240 */
#define FCURVE_240_TONECURVE_BRIGHT_TABLE188_MASK                         0xfff
#define FCURVE_240_TONECURVE_BRIGHT_TABLE188_LSB                              0
#define FCURVE_240_TONECURVE_BRIGHT_TABLE189_MASK                     0xfff0000
#define FCURVE_240_TONECURVE_BRIGHT_TABLE189_LSB                             16

/* ISP_FCURVE_241 */
#define FCURVE_241_TONECURVE_BRIGHT_TABLE190_MASK                         0xfff
#define FCURVE_241_TONECURVE_BRIGHT_TABLE190_LSB                              0
#define FCURVE_241_TONECURVE_BRIGHT_TABLE191_MASK                     0xfff0000
#define FCURVE_241_TONECURVE_BRIGHT_TABLE191_LSB                             16

/* ISP_FCURVE_242 */
#define FCURVE_242_TONECURVE_BRIGHT_TABLE192_MASK                         0xfff
#define FCURVE_242_TONECURVE_BRIGHT_TABLE192_LSB                              0
#define FCURVE_242_TONECURVE_BRIGHT_TABLE193_MASK                     0xfff0000
#define FCURVE_242_TONECURVE_BRIGHT_TABLE193_LSB                             16

/* ISP_FCURVE_243 */
#define FCURVE_243_TONECURVE_BRIGHT_TABLE194_MASK                         0xfff
#define FCURVE_243_TONECURVE_BRIGHT_TABLE194_LSB                              0
#define FCURVE_243_TONECURVE_BRIGHT_TABLE195_MASK                     0xfff0000
#define FCURVE_243_TONECURVE_BRIGHT_TABLE195_LSB                             16

/* ISP_FCURVE_244 */
#define FCURVE_244_TONECURVE_BRIGHT_TABLE196_MASK                         0xfff
#define FCURVE_244_TONECURVE_BRIGHT_TABLE196_LSB                              0
#define FCURVE_244_TONECURVE_BRIGHT_TABLE197_MASK                     0xfff0000
#define FCURVE_244_TONECURVE_BRIGHT_TABLE197_LSB                             16

/* ISP_FCURVE_245 */
#define FCURVE_245_TONECURVE_BRIGHT_TABLE198_MASK                         0xfff
#define FCURVE_245_TONECURVE_BRIGHT_TABLE198_LSB                              0
#define FCURVE_245_TONECURVE_BRIGHT_TABLE199_MASK                     0xfff0000
#define FCURVE_245_TONECURVE_BRIGHT_TABLE199_LSB                             16

/* ISP_FCURVE_246 */
#define FCURVE_246_TONECURVE_BRIGHT_TABLE200_MASK                         0xfff
#define FCURVE_246_TONECURVE_BRIGHT_TABLE200_LSB                              0
#define FCURVE_246_TONECURVE_BRIGHT_TABLE201_MASK                     0xfff0000
#define FCURVE_246_TONECURVE_BRIGHT_TABLE201_LSB                             16

/* ISP_FCURVE_247 */
#define FCURVE_247_TONECURVE_BRIGHT_TABLE202_MASK                         0xfff
#define FCURVE_247_TONECURVE_BRIGHT_TABLE202_LSB                              0
#define FCURVE_247_TONECURVE_BRIGHT_TABLE203_MASK                     0xfff0000
#define FCURVE_247_TONECURVE_BRIGHT_TABLE203_LSB                             16

/* ISP_FCURVE_248 */
#define FCURVE_248_TONECURVE_BRIGHT_TABLE204_MASK                         0xfff
#define FCURVE_248_TONECURVE_BRIGHT_TABLE204_LSB                              0
#define FCURVE_248_TONECURVE_BRIGHT_TABLE205_MASK                     0xfff0000
#define FCURVE_248_TONECURVE_BRIGHT_TABLE205_LSB                             16

/* ISP_FCURVE_249 */
#define FCURVE_249_TONECURVE_BRIGHT_TABLE206_MASK                         0xfff
#define FCURVE_249_TONECURVE_BRIGHT_TABLE206_LSB                              0
#define FCURVE_249_TONECURVE_BRIGHT_TABLE207_MASK                     0xfff0000
#define FCURVE_249_TONECURVE_BRIGHT_TABLE207_LSB                             16

/* ISP_FCURVE_250 */
#define FCURVE_250_TONECURVE_BRIGHT_TABLE208_MASK                         0xfff
#define FCURVE_250_TONECURVE_BRIGHT_TABLE208_LSB                              0
#define FCURVE_250_TONECURVE_BRIGHT_TABLE209_MASK                     0xfff0000
#define FCURVE_250_TONECURVE_BRIGHT_TABLE209_LSB                             16

/* ISP_FCURVE_251 */
#define FCURVE_251_TONECURVE_BRIGHT_TABLE210_MASK                         0xfff
#define FCURVE_251_TONECURVE_BRIGHT_TABLE210_LSB                              0
#define FCURVE_251_TONECURVE_BRIGHT_TABLE211_MASK                     0xfff0000
#define FCURVE_251_TONECURVE_BRIGHT_TABLE211_LSB                             16

/* ISP_FCURVE_252 */
#define FCURVE_252_TONECURVE_BRIGHT_TABLE212_MASK                         0xfff
#define FCURVE_252_TONECURVE_BRIGHT_TABLE212_LSB                              0
#define FCURVE_252_TONECURVE_BRIGHT_TABLE213_MASK                     0xfff0000
#define FCURVE_252_TONECURVE_BRIGHT_TABLE213_LSB                             16

/* ISP_FCURVE_253 */
#define FCURVE_253_TONECURVE_BRIGHT_TABLE214_MASK                         0xfff
#define FCURVE_253_TONECURVE_BRIGHT_TABLE214_LSB                              0
#define FCURVE_253_TONECURVE_BRIGHT_TABLE215_MASK                     0xfff0000
#define FCURVE_253_TONECURVE_BRIGHT_TABLE215_LSB                             16

/* ISP_FCURVE_254 */
#define FCURVE_254_TONECURVE_BRIGHT_TABLE216_MASK                         0xfff
#define FCURVE_254_TONECURVE_BRIGHT_TABLE216_LSB                              0
#define FCURVE_254_TONECURVE_BRIGHT_TABLE217_MASK                     0xfff0000
#define FCURVE_254_TONECURVE_BRIGHT_TABLE217_LSB                             16

/* ISP_FCURVE_255 */
#define FCURVE_255_TONECURVE_BRIGHT_TABLE218_MASK                         0xfff
#define FCURVE_255_TONECURVE_BRIGHT_TABLE218_LSB                              0
#define FCURVE_255_TONECURVE_BRIGHT_TABLE219_MASK                     0xfff0000
#define FCURVE_255_TONECURVE_BRIGHT_TABLE219_LSB                             16

/* ISP_FCURVE_256 */
#define FCURVE_256_TONECURVE_BRIGHT_TABLE220_MASK                         0xfff
#define FCURVE_256_TONECURVE_BRIGHT_TABLE220_LSB                              0
#define FCURVE_256_TONECURVE_BRIGHT_TABLE221_MASK                     0xfff0000
#define FCURVE_256_TONECURVE_BRIGHT_TABLE221_LSB                             16

/* ISP_FCURVE_257 */
#define FCURVE_257_TONECURVE_BRIGHT_TABLE222_MASK                         0xfff
#define FCURVE_257_TONECURVE_BRIGHT_TABLE222_LSB                              0
#define FCURVE_257_TONECURVE_BRIGHT_TABLE223_MASK                     0xfff0000
#define FCURVE_257_TONECURVE_BRIGHT_TABLE223_LSB                             16

/* ISP_FCURVE_258 */
#define FCURVE_258_TONECURVE_BRIGHT_TABLE224_MASK                         0xfff
#define FCURVE_258_TONECURVE_BRIGHT_TABLE224_LSB                              0
#define FCURVE_258_TONECURVE_BRIGHT_TABLE225_MASK                     0xfff0000
#define FCURVE_258_TONECURVE_BRIGHT_TABLE225_LSB                             16

/* ISP_FCURVE_259 */
#define FCURVE_259_TONECURVE_BRIGHT_TABLE226_MASK                         0xfff
#define FCURVE_259_TONECURVE_BRIGHT_TABLE226_LSB                              0
#define FCURVE_259_TONECURVE_BRIGHT_TABLE227_MASK                     0xfff0000
#define FCURVE_259_TONECURVE_BRIGHT_TABLE227_LSB                             16

/* ISP_FCURVE_260 */
#define FCURVE_260_TONECURVE_BRIGHT_TABLE228_MASK                         0xfff
#define FCURVE_260_TONECURVE_BRIGHT_TABLE228_LSB                              0
#define FCURVE_260_TONECURVE_BRIGHT_TABLE229_MASK                     0xfff0000
#define FCURVE_260_TONECURVE_BRIGHT_TABLE229_LSB                             16

/* ISP_FCURVE_261 */
#define FCURVE_261_TONECURVE_BRIGHT_TABLE230_MASK                         0xfff
#define FCURVE_261_TONECURVE_BRIGHT_TABLE230_LSB                              0
#define FCURVE_261_TONECURVE_BRIGHT_TABLE231_MASK                     0xfff0000
#define FCURVE_261_TONECURVE_BRIGHT_TABLE231_LSB                             16

/* ISP_FCURVE_262 */
#define FCURVE_262_TONECURVE_BRIGHT_TABLE232_MASK                         0xfff
#define FCURVE_262_TONECURVE_BRIGHT_TABLE232_LSB                              0
#define FCURVE_262_TONECURVE_BRIGHT_TABLE233_MASK                     0xfff0000
#define FCURVE_262_TONECURVE_BRIGHT_TABLE233_LSB                             16

/* ISP_FCURVE_263 */
#define FCURVE_263_TONECURVE_BRIGHT_TABLE234_MASK                         0xfff
#define FCURVE_263_TONECURVE_BRIGHT_TABLE234_LSB                              0
#define FCURVE_263_TONECURVE_BRIGHT_TABLE235_MASK                     0xfff0000
#define FCURVE_263_TONECURVE_BRIGHT_TABLE235_LSB                             16

/* ISP_FCURVE_264 */
#define FCURVE_264_TONECURVE_BRIGHT_TABLE236_MASK                         0xfff
#define FCURVE_264_TONECURVE_BRIGHT_TABLE236_LSB                              0
#define FCURVE_264_TONECURVE_BRIGHT_TABLE237_MASK                     0xfff0000
#define FCURVE_264_TONECURVE_BRIGHT_TABLE237_LSB                             16

/* ISP_FCURVE_265 */
#define FCURVE_265_TONECURVE_BRIGHT_TABLE238_MASK                         0xfff
#define FCURVE_265_TONECURVE_BRIGHT_TABLE238_LSB                              0
#define FCURVE_265_TONECURVE_BRIGHT_TABLE239_MASK                     0xfff0000
#define FCURVE_265_TONECURVE_BRIGHT_TABLE239_LSB                             16

/* ISP_FCURVE_266 */
#define FCURVE_266_TONECURVE_BRIGHT_TABLE240_MASK                         0xfff
#define FCURVE_266_TONECURVE_BRIGHT_TABLE240_LSB                              0
#define FCURVE_266_TONECURVE_BRIGHT_TABLE241_MASK                     0xfff0000
#define FCURVE_266_TONECURVE_BRIGHT_TABLE241_LSB                             16

/* ISP_FCURVE_267 */
#define FCURVE_267_TONECURVE_BRIGHT_TABLE242_MASK                         0xfff
#define FCURVE_267_TONECURVE_BRIGHT_TABLE242_LSB                              0
#define FCURVE_267_TONECURVE_BRIGHT_TABLE243_MASK                     0xfff0000
#define FCURVE_267_TONECURVE_BRIGHT_TABLE243_LSB                             16

/* ISP_FCURVE_268 */
#define FCURVE_268_TONECURVE_BRIGHT_TABLE244_MASK                         0xfff
#define FCURVE_268_TONECURVE_BRIGHT_TABLE244_LSB                              0
#define FCURVE_268_TONECURVE_BRIGHT_TABLE245_MASK                     0xfff0000
#define FCURVE_268_TONECURVE_BRIGHT_TABLE245_LSB                             16

/* ISP_FCURVE_269 */
#define FCURVE_269_TONECURVE_BRIGHT_TABLE246_MASK                         0xfff
#define FCURVE_269_TONECURVE_BRIGHT_TABLE246_LSB                              0
#define FCURVE_269_TONECURVE_BRIGHT_TABLE247_MASK                     0xfff0000
#define FCURVE_269_TONECURVE_BRIGHT_TABLE247_LSB                             16

/* ISP_FCURVE_270 */
#define FCURVE_270_TONECURVE_BRIGHT_TABLE248_MASK                         0xfff
#define FCURVE_270_TONECURVE_BRIGHT_TABLE248_LSB                              0
#define FCURVE_270_TONECURVE_BRIGHT_TABLE249_MASK                     0xfff0000
#define FCURVE_270_TONECURVE_BRIGHT_TABLE249_LSB                             16

/* ISP_FCURVE_271 */
#define FCURVE_271_TONECURVE_BRIGHT_TABLE250_MASK                         0xfff
#define FCURVE_271_TONECURVE_BRIGHT_TABLE250_LSB                              0
#define FCURVE_271_TONECURVE_BRIGHT_TABLE251_MASK                     0xfff0000
#define FCURVE_271_TONECURVE_BRIGHT_TABLE251_LSB                             16

/* ISP_FCURVE_272 */
#define FCURVE_272_TONECURVE_BRIGHT_TABLE252_MASK                         0xfff
#define FCURVE_272_TONECURVE_BRIGHT_TABLE252_LSB                              0
#define FCURVE_272_TONECURVE_BRIGHT_TABLE253_MASK                     0xfff0000
#define FCURVE_272_TONECURVE_BRIGHT_TABLE253_LSB                             16

/* ISP_FCURVE_273 */
#define FCURVE_273_TONECURVE_BRIGHT_TABLE254_MASK                         0xfff
#define FCURVE_273_TONECURVE_BRIGHT_TABLE254_LSB                              0
#define FCURVE_273_TONECURVE_BRIGHT_TABLE255_MASK                     0xfff0000
#define FCURVE_273_TONECURVE_BRIGHT_TABLE255_LSB                             16

/* ISP_FCURVE_274 */
#define FCURVE_274_TONECURVE_BRIGHT_TABLE256_MASK                         0xfff
#define FCURVE_274_TONECURVE_BRIGHT_TABLE256_LSB                              0

/* ISP_RGBIR_0 */
#define RGBIR_0_IR_BAYER_FORMAT_MASK                                        0x7
#define RGBIR_0_IR_BAYER_FORMAT_LSB                                           0
#define RGBIR_0_RGBIR_DM_CORRB_EN_MASK                                     0x10
#define RGBIR_0_RGBIR_DM_CORRB_EN_LSB                                         4
#define RGBIR_0_RGBIR_DM_RATIO_CORRB_MASK                               0xf0000
#define RGBIR_0_RGBIR_DM_RATIO_CORRB_LSB                                     16
#define RGBIR_0_RGBIR_DM_RATIO_CORIR_MASK                             0xf000000
#define RGBIR_0_RGBIR_DM_RATIO_CORIR_LSB                                     24

/* ISP_RGBIR_1 */
#define RGBIR_1_RGBIR_DM_TH_EDGEDIFF_MASK                                 0xfff
#define RGBIR_1_RGBIR_DM_TH_EDGEDIFF_LSB                                      0
#define RGBIR_1_RGBIR_DM_TH_EDGESUM_MASK                              0xfff0000
#define RGBIR_1_RGBIR_DM_TH_EDGESUM_LSB                                      16

/* ISP_RGBIR_2 */
#define RGBIR_2_RGBIR_IRSUB_OB_MASK                                       0xfff
#define RGBIR_2_RGBIR_IRSUB_OB_LSB                                            0

/* ISP_RGBIR_3 */
#define RGBIR_3_RGBIR_IRSUB_R_LUT0_MASK                                    0xff
#define RGBIR_3_RGBIR_IRSUB_R_LUT0_LSB                                        0
#define RGBIR_3_RGBIR_IRSUB_R_LUT1_MASK                                  0xff00
#define RGBIR_3_RGBIR_IRSUB_R_LUT1_LSB                                        8
#define RGBIR_3_RGBIR_IRSUB_R_LUT2_MASK                                0xff0000
#define RGBIR_3_RGBIR_IRSUB_R_LUT2_LSB                                       16
#define RGBIR_3_RGBIR_IRSUB_R_LUT3_MASK                              0xff000000
#define RGBIR_3_RGBIR_IRSUB_R_LUT3_LSB                                       24

/* ISP_RGBIR_4 */
#define RGBIR_4_RGBIR_IRSUB_R_LUT4_MASK                                    0xff
#define RGBIR_4_RGBIR_IRSUB_R_LUT4_LSB                                        0
#define RGBIR_4_RGBIR_IRSUB_R_LUT5_MASK                                  0xff00
#define RGBIR_4_RGBIR_IRSUB_R_LUT5_LSB                                        8
#define RGBIR_4_RGBIR_IRSUB_R_LUT6_MASK                                0xff0000
#define RGBIR_4_RGBIR_IRSUB_R_LUT6_LSB                                       16
#define RGBIR_4_RGBIR_IRSUB_R_LUT7_MASK                              0xff000000
#define RGBIR_4_RGBIR_IRSUB_R_LUT7_LSB                                       24

/* ISP_RGBIR_5 */
#define RGBIR_5_RGBIR_IRSUB_R_LUT8_MASK                                    0xff
#define RGBIR_5_RGBIR_IRSUB_R_LUT8_LSB                                        0

/* ISP_RGBIR_6 */
#define RGBIR_6_RGBIR_IRSUB_G_LUT0_MASK                                    0xff
#define RGBIR_6_RGBIR_IRSUB_G_LUT0_LSB                                        0
#define RGBIR_6_RGBIR_IRSUB_G_LUT1_MASK                                  0xff00
#define RGBIR_6_RGBIR_IRSUB_G_LUT1_LSB                                        8
#define RGBIR_6_RGBIR_IRSUB_G_LUT2_MASK                                0xff0000
#define RGBIR_6_RGBIR_IRSUB_G_LUT2_LSB                                       16
#define RGBIR_6_RGBIR_IRSUB_G_LUT3_MASK                              0xff000000
#define RGBIR_6_RGBIR_IRSUB_G_LUT3_LSB                                       24

/* ISP_RGBIR_7 */
#define RGBIR_7_RGBIR_IRSUB_G_LUT4_MASK                                    0xff
#define RGBIR_7_RGBIR_IRSUB_G_LUT4_LSB                                        0
#define RGBIR_7_RGBIR_IRSUB_G_LUT5_MASK                                  0xff00
#define RGBIR_7_RGBIR_IRSUB_G_LUT5_LSB                                        8
#define RGBIR_7_RGBIR_IRSUB_G_LUT6_MASK                                0xff0000
#define RGBIR_7_RGBIR_IRSUB_G_LUT6_LSB                                       16
#define RGBIR_7_RGBIR_IRSUB_G_LUT7_MASK                              0xff000000
#define RGBIR_7_RGBIR_IRSUB_G_LUT7_LSB                                       24

/* ISP_RGBIR_8 */
#define RGBIR_8_RGBIR_IRSUB_G_LUT8_MASK                                    0xff
#define RGBIR_8_RGBIR_IRSUB_G_LUT8_LSB                                        0

/* ISP_RGBIR_9 */
#define RGBIR_9_RGBIR_IRSUB_B_LUT0_MASK                                    0xff
#define RGBIR_9_RGBIR_IRSUB_B_LUT0_LSB                                        0
#define RGBIR_9_RGBIR_IRSUB_B_LUT1_MASK                                  0xff00
#define RGBIR_9_RGBIR_IRSUB_B_LUT1_LSB                                        8
#define RGBIR_9_RGBIR_IRSUB_B_LUT2_MASK                                0xff0000
#define RGBIR_9_RGBIR_IRSUB_B_LUT2_LSB                                       16
#define RGBIR_9_RGBIR_IRSUB_B_LUT3_MASK                              0xff000000
#define RGBIR_9_RGBIR_IRSUB_B_LUT3_LSB                                       24

/* ISP_RGBIR_10 */
#define RGBIR_10_RGBIR_IRSUB_B_LUT4_MASK                                   0xff
#define RGBIR_10_RGBIR_IRSUB_B_LUT4_LSB                                       0
#define RGBIR_10_RGBIR_IRSUB_B_LUT5_MASK                                 0xff00
#define RGBIR_10_RGBIR_IRSUB_B_LUT5_LSB                                       8
#define RGBIR_10_RGBIR_IRSUB_B_LUT6_MASK                               0xff0000
#define RGBIR_10_RGBIR_IRSUB_B_LUT6_LSB                                      16
#define RGBIR_10_RGBIR_IRSUB_B_LUT7_MASK                             0xff000000
#define RGBIR_10_RGBIR_IRSUB_B_LUT7_LSB                                      24

/* ISP_RGBIR_11 */
#define RGBIR_11_RGBIR_IRSUB_B_LUT8_MASK                                   0xff
#define RGBIR_11_RGBIR_IRSUB_B_LUT8_LSB                                       0

/* ISP_DPC_0 */
#define DPC_0_DPC_TH_EDGE_MASK                                            0xfff
#define DPC_0_DPC_TH_EDGE_LSB                                                 0

/* ISP_DPC_1 */
#define DPC_1_DPC_BRIGHT_LUT0_MASK                                        0xfff
#define DPC_1_DPC_BRIGHT_LUT0_LSB                                             0
#define DPC_1_DPC_BRIGHT_LUT1_MASK                                    0xfff0000
#define DPC_1_DPC_BRIGHT_LUT1_LSB                                            16

/* ISP_DPC_2 */
#define DPC_2_DPC_BRIGHT_LUT2_MASK                                        0xfff
#define DPC_2_DPC_BRIGHT_LUT2_LSB                                             0
#define DPC_2_DPC_BRIGHT_LUT3_MASK                                    0xfff0000
#define DPC_2_DPC_BRIGHT_LUT3_LSB                                            16

/* ISP_DPC_3 */
#define DPC_3_DPC_BRIGHT_LUT4_MASK                                        0xfff
#define DPC_3_DPC_BRIGHT_LUT4_LSB                                             0

/* ISP_DPC_4 */
#define DPC_4_DPC_DARK_LUT0_MASK                                          0xfff
#define DPC_4_DPC_DARK_LUT0_LSB                                               0
#define DPC_4_DPC_DARK_LUT1_MASK                                      0xfff0000
#define DPC_4_DPC_DARK_LUT1_LSB                                              16

/* ISP_DPC_5 */
#define DPC_5_DPC_DARK_LUT2_MASK                                          0xfff
#define DPC_5_DPC_DARK_LUT2_LSB                                               0
#define DPC_5_DPC_DARK_LUT3_MASK                                      0xfff0000
#define DPC_5_DPC_DARK_LUT3_LSB                                              16

/* ISP_DPC_6 */
#define DPC_6_DPC_DARK_LUT4_MASK                                          0xfff
#define DPC_6_DPC_DARK_LUT4_LSB                                               0

/* ISP_GE_0 */
#define GE_0_GE_GLOBAL_STR_MASK                                            0xff
#define GE_0_GE_GLOBAL_STR_LSB                                                0
#define GE_0_GE_SLOPE_DIFF_MASK                                         0x7ff00
#define GE_0_GE_SLOPE_DIFF_LSB                                                8
#define GE_0_GE_SLOPE_EDGE_MASK                                      0x7ff00000
#define GE_0_GE_SLOPE_EDGE_LSB                                               20

/* ISP_GE_1 */
#define GE_1_GE_DIFF_TH_MASK                                              0xfff
#define GE_1_GE_DIFF_TH_LSB                                                   0
#define GE_1_GE_EDGE_TH_MASK                                          0xfff0000
#define GE_1_GE_EDGE_TH_LSB                                                  16

/* ISP_MLSC_0 */
#define MLSC_0_MLSC_STR_MASK                                              0x1ff
#define MLSC_0_MLSC_STR_LSB                                                   0
#define MLSC_0_MLSC_SCALE_SEL_MASK                                       0x7000
#define MLSC_0_MLSC_SCALE_SEL_LSB                                            12

/* ISP_MLSC_1 */
#define MLSC_1_MLSC_LUT_NUM_X_MASK                                         0xff
#define MLSC_1_MLSC_LUT_NUM_X_LSB                                             0
#define MLSC_1_MLSC_LUT_NUM_Y_MASK                                     0xff0000
#define MLSC_1_MLSC_LUT_NUM_Y_LSB                                            16

/* ISP_MLSC_2 */
#define MLSC_2_MLSC_SCALING_FACT_H_MASK                                  0xffff
#define MLSC_2_MLSC_SCALING_FACT_H_LSB                                        0
#define MLSC_2_MLSC_SCALING_FACT_V_MASK                              0xffff0000
#define MLSC_2_MLSC_SCALING_FACT_V_LSB                                       16

/* ISP_RLSC_0 */
#define RLSC_0_RLSC_STR_MASK                                              0x1ff
#define RLSC_0_RLSC_STR_LSB                                                   0
#define RLSC_0_RLSC_SCALE_SEL_MASK                                       0x7000
#define RLSC_0_RLSC_SCALE_SEL_LSB                                            12
#define RLSC_0_RLSC_NORM_MASK                                        0xffff0000
#define RLSC_0_RLSC_NORM_LSB                                                 16

/* ISP_RLSC_1 */
#define RLSC_1_RLSC_CENTERX_MASK                                         0x1fff
#define RLSC_1_RLSC_CENTERX_LSB                                               0
#define RLSC_1_RLSC_CENTERY_MASK                                     0x1fff0000
#define RLSC_1_RLSC_CENTERY_LSB                                              16

/* ISP_RLSC_2 */
#define RLSC_2_RLSC_GAIN_RLUT0_MASK                                       0xfff
#define RLSC_2_RLSC_GAIN_RLUT0_LSB                                            0
#define RLSC_2_RLSC_GAIN_RLUT1_MASK                                   0xfff0000
#define RLSC_2_RLSC_GAIN_RLUT1_LSB                                           16

/* ISP_RLSC_3 */
#define RLSC_3_RLSC_GAIN_RLUT2_MASK                                       0xfff
#define RLSC_3_RLSC_GAIN_RLUT2_LSB                                            0
#define RLSC_3_RLSC_GAIN_RLUT3_MASK                                   0xfff0000
#define RLSC_3_RLSC_GAIN_RLUT3_LSB                                           16

/* ISP_RLSC_4 */
#define RLSC_4_RLSC_GAIN_RLUT4_MASK                                       0xfff
#define RLSC_4_RLSC_GAIN_RLUT4_LSB                                            0
#define RLSC_4_RLSC_GAIN_RLUT5_MASK                                   0xfff0000
#define RLSC_4_RLSC_GAIN_RLUT5_LSB                                           16

/* ISP_RLSC_5 */
#define RLSC_5_RLSC_GAIN_RLUT6_MASK                                       0xfff
#define RLSC_5_RLSC_GAIN_RLUT6_LSB                                            0
#define RLSC_5_RLSC_GAIN_RLUT7_MASK                                   0xfff0000
#define RLSC_5_RLSC_GAIN_RLUT7_LSB                                           16

/* ISP_RLSC_6 */
#define RLSC_6_RLSC_GAIN_RLUT8_MASK                                       0xfff
#define RLSC_6_RLSC_GAIN_RLUT8_LSB                                            0
#define RLSC_6_RLSC_GAIN_RLUT9_MASK                                   0xfff0000
#define RLSC_6_RLSC_GAIN_RLUT9_LSB                                           16

/* ISP_RLSC_7 */
#define RLSC_7_RLSC_GAIN_RLUT10_MASK                                      0xfff
#define RLSC_7_RLSC_GAIN_RLUT10_LSB                                           0
#define RLSC_7_RLSC_GAIN_RLUT11_MASK                                  0xfff0000
#define RLSC_7_RLSC_GAIN_RLUT11_LSB                                          16

/* ISP_RLSC_8 */
#define RLSC_8_RLSC_GAIN_RLUT12_MASK                                      0xfff
#define RLSC_8_RLSC_GAIN_RLUT12_LSB                                           0
#define RLSC_8_RLSC_GAIN_RLUT13_MASK                                  0xfff0000
#define RLSC_8_RLSC_GAIN_RLUT13_LSB                                          16

/* ISP_RLSC_9 */
#define RLSC_9_RLSC_GAIN_RLUT14_MASK                                      0xfff
#define RLSC_9_RLSC_GAIN_RLUT14_LSB                                           0
#define RLSC_9_RLSC_GAIN_RLUT15_MASK                                  0xfff0000
#define RLSC_9_RLSC_GAIN_RLUT15_LSB                                          16

/* ISP_RLSC_10 */
#define RLSC_10_RLSC_GAIN_RLUT16_MASK                                     0xfff
#define RLSC_10_RLSC_GAIN_RLUT16_LSB                                          0
#define RLSC_10_RLSC_GAIN_RLUT17_MASK                                 0xfff0000
#define RLSC_10_RLSC_GAIN_RLUT17_LSB                                         16

/* ISP_RLSC_11 */
#define RLSC_11_RLSC_GAIN_RLUT18_MASK                                     0xfff
#define RLSC_11_RLSC_GAIN_RLUT18_LSB                                          0
#define RLSC_11_RLSC_GAIN_RLUT19_MASK                                 0xfff0000
#define RLSC_11_RLSC_GAIN_RLUT19_LSB                                         16

/* ISP_RLSC_12 */
#define RLSC_12_RLSC_GAIN_RLUT20_MASK                                     0xfff
#define RLSC_12_RLSC_GAIN_RLUT20_LSB                                          0
#define RLSC_12_RLSC_GAIN_RLUT21_MASK                                 0xfff0000
#define RLSC_12_RLSC_GAIN_RLUT21_LSB                                         16

/* ISP_RLSC_13 */
#define RLSC_13_RLSC_GAIN_RLUT22_MASK                                     0xfff
#define RLSC_13_RLSC_GAIN_RLUT22_LSB                                          0
#define RLSC_13_RLSC_GAIN_RLUT23_MASK                                 0xfff0000
#define RLSC_13_RLSC_GAIN_RLUT23_LSB                                         16

/* ISP_RLSC_14 */
#define RLSC_14_RLSC_GAIN_RLUT24_MASK                                     0xfff
#define RLSC_14_RLSC_GAIN_RLUT24_LSB                                          0
#define RLSC_14_RLSC_GAIN_RLUT25_MASK                                 0xfff0000
#define RLSC_14_RLSC_GAIN_RLUT25_LSB                                         16

/* ISP_RLSC_15 */
#define RLSC_15_RLSC_GAIN_RLUT26_MASK                                     0xfff
#define RLSC_15_RLSC_GAIN_RLUT26_LSB                                          0
#define RLSC_15_RLSC_GAIN_RLUT27_MASK                                 0xfff0000
#define RLSC_15_RLSC_GAIN_RLUT27_LSB                                         16

/* ISP_RLSC_16 */
#define RLSC_16_RLSC_GAIN_RLUT28_MASK                                     0xfff
#define RLSC_16_RLSC_GAIN_RLUT28_LSB                                          0
#define RLSC_16_RLSC_GAIN_RLUT29_MASK                                 0xfff0000
#define RLSC_16_RLSC_GAIN_RLUT29_LSB                                         16

/* ISP_RLSC_17 */
#define RLSC_17_RLSC_GAIN_RLUT30_MASK                                     0xfff
#define RLSC_17_RLSC_GAIN_RLUT30_LSB                                          0
#define RLSC_17_RLSC_GAIN_RLUT31_MASK                                 0xfff0000
#define RLSC_17_RLSC_GAIN_RLUT31_LSB                                         16

/* ISP_RLSC_18 */
#define RLSC_18_RLSC_GAIN_RLUT32_MASK                                     0xfff
#define RLSC_18_RLSC_GAIN_RLUT32_LSB                                          0

/* ISP_RLSC_19 */
#define RLSC_19_RLSC_GAIN_GLUT0_MASK                                      0xfff
#define RLSC_19_RLSC_GAIN_GLUT0_LSB                                           0
#define RLSC_19_RLSC_GAIN_GLUT1_MASK                                  0xfff0000
#define RLSC_19_RLSC_GAIN_GLUT1_LSB                                          16

/* ISP_RLSC_20 */
#define RLSC_20_RLSC_GAIN_GLUT2_MASK                                      0xfff
#define RLSC_20_RLSC_GAIN_GLUT2_LSB                                           0
#define RLSC_20_RLSC_GAIN_GLUT3_MASK                                  0xfff0000
#define RLSC_20_RLSC_GAIN_GLUT3_LSB                                          16

/* ISP_RLSC_21 */
#define RLSC_21_RLSC_GAIN_GLUT4_MASK                                      0xfff
#define RLSC_21_RLSC_GAIN_GLUT4_LSB                                           0
#define RLSC_21_RLSC_GAIN_GLUT5_MASK                                  0xfff0000
#define RLSC_21_RLSC_GAIN_GLUT5_LSB                                          16

/* ISP_RLSC_22 */
#define RLSC_22_RLSC_GAIN_GLUT6_MASK                                      0xfff
#define RLSC_22_RLSC_GAIN_GLUT6_LSB                                           0
#define RLSC_22_RLSC_GAIN_GLUT7_MASK                                  0xfff0000
#define RLSC_22_RLSC_GAIN_GLUT7_LSB                                          16

/* ISP_RLSC_23 */
#define RLSC_23_RLSC_GAIN_GLUT8_MASK                                      0xfff
#define RLSC_23_RLSC_GAIN_GLUT8_LSB                                           0
#define RLSC_23_RLSC_GAIN_GLUT9_MASK                                  0xfff0000
#define RLSC_23_RLSC_GAIN_GLUT9_LSB                                          16

/* ISP_RLSC_24 */
#define RLSC_24_RLSC_GAIN_GLUT10_MASK                                     0xfff
#define RLSC_24_RLSC_GAIN_GLUT10_LSB                                          0
#define RLSC_24_RLSC_GAIN_GLUT11_MASK                                 0xfff0000
#define RLSC_24_RLSC_GAIN_GLUT11_LSB                                         16

/* ISP_RLSC_25 */
#define RLSC_25_RLSC_GAIN_GLUT12_MASK                                     0xfff
#define RLSC_25_RLSC_GAIN_GLUT12_LSB                                          0
#define RLSC_25_RLSC_GAIN_GLUT13_MASK                                 0xfff0000
#define RLSC_25_RLSC_GAIN_GLUT13_LSB                                         16

/* ISP_RLSC_26 */
#define RLSC_26_RLSC_GAIN_GLUT14_MASK                                     0xfff
#define RLSC_26_RLSC_GAIN_GLUT14_LSB                                          0
#define RLSC_26_RLSC_GAIN_GLUT15_MASK                                 0xfff0000
#define RLSC_26_RLSC_GAIN_GLUT15_LSB                                         16

/* ISP_RLSC_27 */
#define RLSC_27_RLSC_GAIN_GLUT16_MASK                                     0xfff
#define RLSC_27_RLSC_GAIN_GLUT16_LSB                                          0
#define RLSC_27_RLSC_GAIN_GLUT17_MASK                                 0xfff0000
#define RLSC_27_RLSC_GAIN_GLUT17_LSB                                         16

/* ISP_RLSC_28 */
#define RLSC_28_RLSC_GAIN_GLUT18_MASK                                     0xfff
#define RLSC_28_RLSC_GAIN_GLUT18_LSB                                          0
#define RLSC_28_RLSC_GAIN_GLUT19_MASK                                 0xfff0000
#define RLSC_28_RLSC_GAIN_GLUT19_LSB                                         16

/* ISP_RLSC_29 */
#define RLSC_29_RLSC_GAIN_GLUT20_MASK                                     0xfff
#define RLSC_29_RLSC_GAIN_GLUT20_LSB                                          0
#define RLSC_29_RLSC_GAIN_GLUT21_MASK                                 0xfff0000
#define RLSC_29_RLSC_GAIN_GLUT21_LSB                                         16

/* ISP_RLSC_30 */
#define RLSC_30_RLSC_GAIN_GLUT22_MASK                                     0xfff
#define RLSC_30_RLSC_GAIN_GLUT22_LSB                                          0
#define RLSC_30_RLSC_GAIN_GLUT23_MASK                                 0xfff0000
#define RLSC_30_RLSC_GAIN_GLUT23_LSB                                         16

/* ISP_RLSC_31 */
#define RLSC_31_RLSC_GAIN_GLUT24_MASK                                     0xfff
#define RLSC_31_RLSC_GAIN_GLUT24_LSB                                          0
#define RLSC_31_RLSC_GAIN_GLUT25_MASK                                 0xfff0000
#define RLSC_31_RLSC_GAIN_GLUT25_LSB                                         16

/* ISP_RLSC_32 */
#define RLSC_32_RLSC_GAIN_GLUT26_MASK                                     0xfff
#define RLSC_32_RLSC_GAIN_GLUT26_LSB                                          0
#define RLSC_32_RLSC_GAIN_GLUT27_MASK                                 0xfff0000
#define RLSC_32_RLSC_GAIN_GLUT27_LSB                                         16

/* ISP_RLSC_33 */
#define RLSC_33_RLSC_GAIN_GLUT28_MASK                                     0xfff
#define RLSC_33_RLSC_GAIN_GLUT28_LSB                                          0
#define RLSC_33_RLSC_GAIN_GLUT29_MASK                                 0xfff0000
#define RLSC_33_RLSC_GAIN_GLUT29_LSB                                         16

/* ISP_RLSC_34 */
#define RLSC_34_RLSC_GAIN_GLUT30_MASK                                     0xfff
#define RLSC_34_RLSC_GAIN_GLUT30_LSB                                          0
#define RLSC_34_RLSC_GAIN_GLUT31_MASK                                 0xfff0000
#define RLSC_34_RLSC_GAIN_GLUT31_LSB                                         16

/* ISP_RLSC_35 */
#define RLSC_35_RLSC_GAIN_GLUT32_MASK                                     0xfff
#define RLSC_35_RLSC_GAIN_GLUT32_LSB                                          0

/* ISP_RLSC_36 */
#define RLSC_36_RLSC_GAIN_BLUT0_MASK                                      0xfff
#define RLSC_36_RLSC_GAIN_BLUT0_LSB                                           0
#define RLSC_36_RLSC_GAIN_BLUT1_MASK                                  0xfff0000
#define RLSC_36_RLSC_GAIN_BLUT1_LSB                                          16

/* ISP_RLSC_37 */
#define RLSC_37_RLSC_GAIN_BLUT2_MASK                                      0xfff
#define RLSC_37_RLSC_GAIN_BLUT2_LSB                                           0
#define RLSC_37_RLSC_GAIN_BLUT3_MASK                                  0xfff0000
#define RLSC_37_RLSC_GAIN_BLUT3_LSB                                          16

/* ISP_RLSC_38 */
#define RLSC_38_RLSC_GAIN_BLUT4_MASK                                      0xfff
#define RLSC_38_RLSC_GAIN_BLUT4_LSB                                           0
#define RLSC_38_RLSC_GAIN_BLUT5_MASK                                  0xfff0000
#define RLSC_38_RLSC_GAIN_BLUT5_LSB                                          16

/* ISP_RLSC_39 */
#define RLSC_39_RLSC_GAIN_BLUT6_MASK                                      0xfff
#define RLSC_39_RLSC_GAIN_BLUT6_LSB                                           0
#define RLSC_39_RLSC_GAIN_BLUT7_MASK                                  0xfff0000
#define RLSC_39_RLSC_GAIN_BLUT7_LSB                                          16

/* ISP_RLSC_40 */
#define RLSC_40_RLSC_GAIN_BLUT8_MASK                                      0xfff
#define RLSC_40_RLSC_GAIN_BLUT8_LSB                                           0
#define RLSC_40_RLSC_GAIN_BLUT9_MASK                                  0xfff0000
#define RLSC_40_RLSC_GAIN_BLUT9_LSB                                          16

/* ISP_RLSC_41 */
#define RLSC_41_RLSC_GAIN_BLUT10_MASK                                     0xfff
#define RLSC_41_RLSC_GAIN_BLUT10_LSB                                          0
#define RLSC_41_RLSC_GAIN_BLUT11_MASK                                 0xfff0000
#define RLSC_41_RLSC_GAIN_BLUT11_LSB                                         16

/* ISP_RLSC_42 */
#define RLSC_42_RLSC_GAIN_BLUT12_MASK                                     0xfff
#define RLSC_42_RLSC_GAIN_BLUT12_LSB                                          0
#define RLSC_42_RLSC_GAIN_BLUT13_MASK                                 0xfff0000
#define RLSC_42_RLSC_GAIN_BLUT13_LSB                                         16

/* ISP_RLSC_43 */
#define RLSC_43_RLSC_GAIN_BLUT14_MASK                                     0xfff
#define RLSC_43_RLSC_GAIN_BLUT14_LSB                                          0
#define RLSC_43_RLSC_GAIN_BLUT15_MASK                                 0xfff0000
#define RLSC_43_RLSC_GAIN_BLUT15_LSB                                         16

/* ISP_RLSC_44 */
#define RLSC_44_RLSC_GAIN_BLUT16_MASK                                     0xfff
#define RLSC_44_RLSC_GAIN_BLUT16_LSB                                          0
#define RLSC_44_RLSC_GAIN_BLUT17_MASK                                 0xfff0000
#define RLSC_44_RLSC_GAIN_BLUT17_LSB                                         16

/* ISP_RLSC_45 */
#define RLSC_45_RLSC_GAIN_BLUT18_MASK                                     0xfff
#define RLSC_45_RLSC_GAIN_BLUT18_LSB                                          0
#define RLSC_45_RLSC_GAIN_BLUT19_MASK                                 0xfff0000
#define RLSC_45_RLSC_GAIN_BLUT19_LSB                                         16

/* ISP_RLSC_46 */
#define RLSC_46_RLSC_GAIN_BLUT20_MASK                                     0xfff
#define RLSC_46_RLSC_GAIN_BLUT20_LSB                                          0
#define RLSC_46_RLSC_GAIN_BLUT21_MASK                                 0xfff0000
#define RLSC_46_RLSC_GAIN_BLUT21_LSB                                         16

/* ISP_RLSC_47 */
#define RLSC_47_RLSC_GAIN_BLUT22_MASK                                     0xfff
#define RLSC_47_RLSC_GAIN_BLUT22_LSB                                          0
#define RLSC_47_RLSC_GAIN_BLUT23_MASK                                 0xfff0000
#define RLSC_47_RLSC_GAIN_BLUT23_LSB                                         16

/* ISP_RLSC_48 */
#define RLSC_48_RLSC_GAIN_BLUT24_MASK                                     0xfff
#define RLSC_48_RLSC_GAIN_BLUT24_LSB                                          0
#define RLSC_48_RLSC_GAIN_BLUT25_MASK                                 0xfff0000
#define RLSC_48_RLSC_GAIN_BLUT25_LSB                                         16

/* ISP_RLSC_49 */
#define RLSC_49_RLSC_GAIN_BLUT26_MASK                                     0xfff
#define RLSC_49_RLSC_GAIN_BLUT26_LSB                                          0
#define RLSC_49_RLSC_GAIN_BLUT27_MASK                                 0xfff0000
#define RLSC_49_RLSC_GAIN_BLUT27_LSB                                         16

/* ISP_RLSC_50 */
#define RLSC_50_RLSC_GAIN_BLUT28_MASK                                     0xfff
#define RLSC_50_RLSC_GAIN_BLUT28_LSB                                          0
#define RLSC_50_RLSC_GAIN_BLUT29_MASK                                 0xfff0000
#define RLSC_50_RLSC_GAIN_BLUT29_LSB                                         16

/* ISP_RLSC_51 */
#define RLSC_51_RLSC_GAIN_BLUT30_MASK                                     0xfff
#define RLSC_51_RLSC_GAIN_BLUT30_LSB                                          0
#define RLSC_51_RLSC_GAIN_BLUT31_MASK                                 0xfff0000
#define RLSC_51_RLSC_GAIN_BLUT31_LSB                                         16

/* ISP_RLSC_52 */
#define RLSC_52_RLSC_GAIN_BLUT32_MASK                                     0xfff
#define RLSC_52_RLSC_GAIN_BLUT32_LSB                                          0

/* ISP_BNR_0 */
#define BNR_0_BNR_SNR_FILT_W0_MASK                                         0x1f
#define BNR_0_BNR_SNR_FILT_W0_LSB                                             0
#define BNR_0_BNR_SNR_FILT_W1_MASK                                       0x1f00
#define BNR_0_BNR_SNR_FILT_W1_LSB                                             8
#define BNR_0_BNR_SNR_FILT_W2_MASK                                     0x1f0000
#define BNR_0_BNR_SNR_FILT_W2_LSB                                            16
#define BNR_0_BNR_SNR_FILT_W3_MASK                                   0x1f000000
#define BNR_0_BNR_SNR_FILT_W3_LSB                                            24

/* ISP_BNR_1 */
#define BNR_1_BNR_SNR_FILT_W4_MASK                                         0x1f
#define BNR_1_BNR_SNR_FILT_W4_LSB                                             0
#define BNR_1_BNR_SNR_FILT_W5_MASK                                       0x1f00
#define BNR_1_BNR_SNR_FILT_W5_LSB                                             8

/* ISP_BNR_2 */
#define BNR_2_BNR_PATCH_DIFF_W0_MASK                                        0x7
#define BNR_2_BNR_PATCH_DIFF_W0_LSB                                           0
#define BNR_2_BNR_PATCH_DIFF_W1_MASK                                       0x70
#define BNR_2_BNR_PATCH_DIFF_W1_LSB                                           4
#define BNR_2_BNR_PATCH_DIFF_W2_MASK                                      0x700
#define BNR_2_BNR_PATCH_DIFF_W2_LSB                                           8
#define BNR_2_BNR_PATCH_DIFF_W3_MASK                                     0x7000
#define BNR_2_BNR_PATCH_DIFF_W3_LSB                                          12
#define BNR_2_BNR_PATCH_DIFF_W4_MASK                                    0x70000
#define BNR_2_BNR_PATCH_DIFF_W4_LSB                                          16
#define BNR_2_BNR_PATCH_NORM_DIV_MASK                                0xfff80000
#define BNR_2_BNR_PATCH_NORM_DIV_LSB                                         19

/* ISP_BNR_3 */
#define BNR_3_BNR_WT_NLM_MASK                                              0x3f
#define BNR_3_BNR_WT_NLM_LSB                                                  0
#define BNR_3_BNR_AC_MUL_MASK                                            0xff00
#define BNR_3_BNR_AC_MUL_LSB                                                  8
#define BNR_3_BNR_AC_CLAMP_MASK                                       0xfff0000
#define BNR_3_BNR_AC_CLAMP_LSB                                               16

/* ISP_BNR_4 */
#define BNR_4_BNR_CENMOD_TH1_MASK                                         0x3ff
#define BNR_4_BNR_CENMOD_TH1_LSB                                              0
#define BNR_4_BNR_CENMOD_TH2_MASK                                     0x3ff0000
#define BNR_4_BNR_CENMOD_TH2_LSB                                             16

/* ISP_BNR_5 */
#define BNR_5_BNR_BILAT_RANGE_LUT0_MASK                                   0x3ff
#define BNR_5_BNR_BILAT_RANGE_LUT0_LSB                                        0
#define BNR_5_BNR_BILAT_RANGE_LUT1_MASK                                 0xffc00
#define BNR_5_BNR_BILAT_RANGE_LUT1_LSB                                       10
#define BNR_5_BNR_BILAT_RANGE_LUT2_MASK                              0x3ff00000
#define BNR_5_BNR_BILAT_RANGE_LUT2_LSB                                       20

/* ISP_BNR_6 */
#define BNR_6_BNR_BILAT_RANGE_LUT3_MASK                                   0x3ff
#define BNR_6_BNR_BILAT_RANGE_LUT3_LSB                                        0
#define BNR_6_BNR_BILAT_RANGE_LUT4_MASK                                 0xffc00
#define BNR_6_BNR_BILAT_RANGE_LUT4_LSB                                       10
#define BNR_6_BNR_BILAT_RANGE_LUT5_MASK                              0x3ff00000
#define BNR_6_BNR_BILAT_RANGE_LUT5_LSB                                       20

/* ISP_BNR_7 */
#define BNR_7_BNR_NLM_RANGE_LUT0_MASK                                     0x3ff
#define BNR_7_BNR_NLM_RANGE_LUT0_LSB                                          0
#define BNR_7_BNR_NLM_RANGE_LUT1_MASK                                   0xffc00
#define BNR_7_BNR_NLM_RANGE_LUT1_LSB                                         10
#define BNR_7_BNR_NLM_RANGE_LUT2_MASK                                0x3ff00000
#define BNR_7_BNR_NLM_RANGE_LUT2_LSB                                         20

/* ISP_BNR_8 */
#define BNR_8_BNR_NLM_RANGE_LUT3_MASK                                     0x3ff
#define BNR_8_BNR_NLM_RANGE_LUT3_LSB                                          0
#define BNR_8_BNR_NLM_RANGE_LUT4_MASK                                   0xffc00
#define BNR_8_BNR_NLM_RANGE_LUT4_LSB                                         10
#define BNR_8_BNR_NLM_RANGE_LUT5_MASK                                0x3ff00000
#define BNR_8_BNR_NLM_RANGE_LUT5_LSB                                         20

/* ISP_BNR_9 */
#define BNR_9_BNR_BILAT_LUM_LUT0_MASK                                     0x3ff
#define BNR_9_BNR_BILAT_LUM_LUT0_LSB                                          0
#define BNR_9_BNR_BILAT_LUM_LUT1_MASK                                   0xffc00
#define BNR_9_BNR_BILAT_LUM_LUT1_LSB                                         10
#define BNR_9_BNR_BILAT_LUM_LUT2_MASK                                0x3ff00000
#define BNR_9_BNR_BILAT_LUM_LUT2_LSB                                         20

/* ISP_BNR_10 */
#define BNR_10_BNR_BILAT_LUM_LUT3_MASK                                    0x3ff
#define BNR_10_BNR_BILAT_LUM_LUT3_LSB                                         0
#define BNR_10_BNR_BILAT_LUM_LUT4_MASK                                  0xffc00
#define BNR_10_BNR_BILAT_LUM_LUT4_LSB                                        10
#define BNR_10_BNR_BILAT_LUM_LUT5_MASK                               0x3ff00000
#define BNR_10_BNR_BILAT_LUM_LUT5_LSB                                        20

/* ISP_BNR_11 */
#define BNR_11_BNR_BILAT_LUM_LUT6_MASK                                    0x3ff
#define BNR_11_BNR_BILAT_LUM_LUT6_LSB                                         0
#define BNR_11_BNR_BILAT_LUM_LUT7_MASK                                  0xffc00
#define BNR_11_BNR_BILAT_LUM_LUT7_LSB                                        10
#define BNR_11_BNR_BILAT_LUM_LUT8_MASK                               0x3ff00000
#define BNR_11_BNR_BILAT_LUM_LUT8_LSB                                        20

/* ISP_BNR_12 */
#define BNR_12_BNR_BILAT_LUM_LUT9_MASK                                    0x3ff
#define BNR_12_BNR_BILAT_LUM_LUT9_LSB                                         0
#define BNR_12_BNR_BILAT_LUM_LUT10_MASK                                 0xffc00
#define BNR_12_BNR_BILAT_LUM_LUT10_LSB                                       10
#define BNR_12_BNR_BILAT_LUM_LUT11_MASK                              0x3ff00000
#define BNR_12_BNR_BILAT_LUM_LUT11_LSB                                       20

/* ISP_BNR_13 */
#define BNR_13_BNR_BILAT_LUM_LUT12_MASK                                   0x3ff
#define BNR_13_BNR_BILAT_LUM_LUT12_LSB                                        0
#define BNR_13_BNR_BILAT_LUM_LUT13_MASK                                 0xffc00
#define BNR_13_BNR_BILAT_LUM_LUT13_LSB                                       10
#define BNR_13_BNR_BILAT_LUM_LUT14_MASK                              0x3ff00000
#define BNR_13_BNR_BILAT_LUM_LUT14_LSB                                       20

/* ISP_BNR_14 */
#define BNR_14_BNR_BILAT_LUM_LUT15_MASK                                   0x3ff
#define BNR_14_BNR_BILAT_LUM_LUT15_LSB                                        0
#define BNR_14_BNR_BILAT_LUM_LUT16_MASK                                 0xffc00
#define BNR_14_BNR_BILAT_LUM_LUT16_LSB                                       10

/* ISP_BNR_15 */
#define BNR_15_BNR_NLM_LUM_LUT0_MASK                                      0x3ff
#define BNR_15_BNR_NLM_LUM_LUT0_LSB                                           0
#define BNR_15_BNR_NLM_LUM_LUT1_MASK                                    0xffc00
#define BNR_15_BNR_NLM_LUM_LUT1_LSB                                          10
#define BNR_15_BNR_NLM_LUM_LUT2_MASK                                 0x3ff00000
#define BNR_15_BNR_NLM_LUM_LUT2_LSB                                          20

/* ISP_BNR_16 */
#define BNR_16_BNR_NLM_LUM_LUT3_MASK                                      0x3ff
#define BNR_16_BNR_NLM_LUM_LUT3_LSB                                           0
#define BNR_16_BNR_NLM_LUM_LUT4_MASK                                    0xffc00
#define BNR_16_BNR_NLM_LUM_LUT4_LSB                                          10
#define BNR_16_BNR_NLM_LUM_LUT5_MASK                                 0x3ff00000
#define BNR_16_BNR_NLM_LUM_LUT5_LSB                                          20

/* ISP_BNR_17 */
#define BNR_17_BNR_NLM_LUM_LUT6_MASK                                      0x3ff
#define BNR_17_BNR_NLM_LUM_LUT6_LSB                                           0
#define BNR_17_BNR_NLM_LUM_LUT7_MASK                                    0xffc00
#define BNR_17_BNR_NLM_LUM_LUT7_LSB                                          10
#define BNR_17_BNR_NLM_LUM_LUT8_MASK                                 0x3ff00000
#define BNR_17_BNR_NLM_LUM_LUT8_LSB                                          20

/* ISP_BNR_18 */
#define BNR_18_BNR_NLM_LUM_LUT9_MASK                                      0x3ff
#define BNR_18_BNR_NLM_LUM_LUT9_LSB                                           0
#define BNR_18_BNR_NLM_LUM_LUT10_MASK                                   0xffc00
#define BNR_18_BNR_NLM_LUM_LUT10_LSB                                         10
#define BNR_18_BNR_NLM_LUM_LUT11_MASK                                0x3ff00000
#define BNR_18_BNR_NLM_LUM_LUT11_LSB                                         20

/* ISP_BNR_19 */
#define BNR_19_BNR_NLM_LUM_LUT12_MASK                                     0x3ff
#define BNR_19_BNR_NLM_LUM_LUT12_LSB                                          0
#define BNR_19_BNR_NLM_LUM_LUT13_MASK                                   0xffc00
#define BNR_19_BNR_NLM_LUM_LUT13_LSB                                         10
#define BNR_19_BNR_NLM_LUM_LUT14_MASK                                0x3ff00000
#define BNR_19_BNR_NLM_LUM_LUT14_LSB                                         20

/* ISP_BNR_20 */
#define BNR_20_BNR_NLM_LUM_LUT15_MASK                                     0x3ff
#define BNR_20_BNR_NLM_LUM_LUT15_LSB                                          0
#define BNR_20_BNR_NLM_LUM_LUT16_MASK                                   0xffc00
#define BNR_20_BNR_NLM_LUM_LUT16_LSB                                         10

/* ISP_WB_0 */
#define WB_0_WB_COLORGAIN_R_MASK                                         0x1fff
#define WB_0_WB_COLORGAIN_R_LSB                                               0
#define WB_0_WB_COLORGAIN_GR_MASK                                    0x1fff0000
#define WB_0_WB_COLORGAIN_GR_LSB                                             16

/* ISP_WB_1 */
#define WB_1_WB_COLORGAIN_GB_MASK                                        0x1fff
#define WB_1_WB_COLORGAIN_GB_LSB                                              0
#define WB_1_WB_COLORGAIN_B_MASK                                     0x1fff0000
#define WB_1_WB_COLORGAIN_B_LSB                                              16

/* ISP_WB_2 */
#define WB_2_WB_OB_R_MASK                                                 0xfff
#define WB_2_WB_OB_R_LSB                                                      0
#define WB_2_WB_OB_GR_MASK                                            0xfff0000
#define WB_2_WB_OB_GR_LSB                                                    16

/* ISP_WB_3 */
#define WB_3_WB_OB_GB_MASK                                                0xfff
#define WB_3_WB_OB_GB_LSB                                                     0
#define WB_3_WB_OB_B_MASK                                             0xfff0000
#define WB_3_WB_OB_B_LSB                                                     16

/* ISP_WDR_0 */
#define WDR_0_WDR_SUB_WIDTH_MASK                                           0x3f
#define WDR_0_WDR_SUB_WIDTH_LSB                                               0
#define WDR_0_WDR_SUB_HEIGHT_MASK                                      0x3f0000
#define WDR_0_WDR_SUB_HEIGHT_LSB                                             16

/* ISP_WDR_1 */
#define WDR_1_WDR_BLK_STCS_FACT_H_MASK                                 0xffffff
#define WDR_1_WDR_BLK_STCS_FACT_H_LSB                                         0

/* ISP_WDR_2 */
#define WDR_2_WDR_BLK_STCS_FACT_V_MASK                                 0xffffff
#define WDR_2_WDR_BLK_STCS_FACT_V_LSB                                         0

/* ISP_WDR_3 */
#define WDR_3_WDR_SCALING_FACT_H_MASK                                    0xffff
#define WDR_3_WDR_SCALING_FACT_H_LSB                                          0
#define WDR_3_WDR_SCALING_FACT_V_MASK                                0xffff0000
#define WDR_3_WDR_SCALING_FACT_V_LSB                                         16

/* ISP_WDR_4 */
#define WDR_4_WDR_BLK_SIZE_H_MASK                                         0x3ff
#define WDR_4_WDR_BLK_SIZE_H_LSB                                              0
#define WDR_4_WDR_BLK_SIZE_V_MASK                                     0x3ff0000
#define WDR_4_WDR_BLK_SIZE_V_LSB                                             16

/* ISP_WDR_5 */
#define WDR_5_WDR_BLK_SIZE_NORM_DIV_MASK                                0xfffff
#define WDR_5_WDR_BLK_SIZE_NORM_DIV_LSB                                       0

/* ISP_WDR_6 */
#define WDR_6_WDR_SUBIN_FILT_W0_MASK                                        0xf
#define WDR_6_WDR_SUBIN_FILT_W0_LSB                                           0
#define WDR_6_WDR_SUBIN_FILT_W1_MASK                                       0xf0
#define WDR_6_WDR_SUBIN_FILT_W1_LSB                                           4
#define WDR_6_WDR_SUBIN_FILT_W2_MASK                                      0xf00
#define WDR_6_WDR_SUBIN_FILT_W2_LSB                                           8
#define WDR_6_WDR_SUBIN_FILT_NORM_DIV_MASK                            0xfff0000
#define WDR_6_WDR_SUBIN_FILT_NORM_DIV_LSB                                    16

/* ISP_WDR_7 */
#define WDR_7_WDR_WT_YMAX_MASK                                              0xf
#define WDR_7_WDR_WT_YMAX_LSB                                                 0

/* ISP_WDR_8 */
#define WDR_8_WDR_WT_BLD_LUT0_MASK                                         0xff
#define WDR_8_WDR_WT_BLD_LUT0_LSB                                             0
#define WDR_8_WDR_WT_BLD_LUT1_MASK                                       0xff00
#define WDR_8_WDR_WT_BLD_LUT1_LSB                                             8
#define WDR_8_WDR_WT_BLD_LUT2_MASK                                     0xff0000
#define WDR_8_WDR_WT_BLD_LUT2_LSB                                            16
#define WDR_8_WDR_WT_BLD_LUT3_MASK                                   0xff000000
#define WDR_8_WDR_WT_BLD_LUT3_LSB                                            24

/* ISP_WDR_9 */
#define WDR_9_WDR_WT_BLD_LUT4_MASK                                         0xff
#define WDR_9_WDR_WT_BLD_LUT4_LSB                                             0
#define WDR_9_WDR_WT_BLD_LUT5_MASK                                       0xff00
#define WDR_9_WDR_WT_BLD_LUT5_LSB                                             8
#define WDR_9_WDR_WT_BLD_LUT6_MASK                                     0xff0000
#define WDR_9_WDR_WT_BLD_LUT6_LSB                                            16
#define WDR_9_WDR_WT_BLD_LUT7_MASK                                   0xff000000
#define WDR_9_WDR_WT_BLD_LUT7_LSB                                            24

/* ISP_WDR_10 */
#define WDR_10_WDR_WT_BLD_LUT8_MASK                                        0xff
#define WDR_10_WDR_WT_BLD_LUT8_LSB                                            0
#define WDR_10_WDR_WT_BLD_LUT9_MASK                                      0xff00
#define WDR_10_WDR_WT_BLD_LUT9_LSB                                            8
#define WDR_10_WDR_WT_BLD_LUT10_MASK                                   0xff0000
#define WDR_10_WDR_WT_BLD_LUT10_LSB                                          16
#define WDR_10_WDR_WT_BLD_LUT11_MASK                                 0xff000000
#define WDR_10_WDR_WT_BLD_LUT11_LSB                                          24

/* ISP_WDR_11 */
#define WDR_11_WDR_WT_BLD_LUT12_MASK                                       0xff
#define WDR_11_WDR_WT_BLD_LUT12_LSB                                           0
#define WDR_11_WDR_WT_BLD_LUT13_MASK                                     0xff00
#define WDR_11_WDR_WT_BLD_LUT13_LSB                                           8
#define WDR_11_WDR_WT_BLD_LUT14_MASK                                   0xff0000
#define WDR_11_WDR_WT_BLD_LUT14_LSB                                          16
#define WDR_11_WDR_WT_BLD_LUT15_MASK                                 0xff000000
#define WDR_11_WDR_WT_BLD_LUT15_LSB                                          24

/* ISP_WDR_12 */
#define WDR_12_WDR_WT_BLD_LUT16_MASK                                       0xff
#define WDR_12_WDR_WT_BLD_LUT16_LSB                                           0

/* ISP_WDR_13 */
#define WDR_13_WDR_DIFF_DARKEN0_MASK                                       0xff
#define WDR_13_WDR_DIFF_DARKEN0_LSB                                           0
#define WDR_13_WDR_DIFF_DARKEN1_MASK                                     0xff00
#define WDR_13_WDR_DIFF_DARKEN1_LSB                                           8
#define WDR_13_WDR_DIFF_DARKEN2_MASK                                   0xff0000
#define WDR_13_WDR_DIFF_DARKEN2_LSB                                          16
#define WDR_13_WDR_DIFF_DARKEN3_MASK                                 0xff000000
#define WDR_13_WDR_DIFF_DARKEN3_LSB                                          24

/* ISP_WDR_14 */
#define WDR_14_WDR_DIFF_DARKEN4_MASK                                       0xff
#define WDR_14_WDR_DIFF_DARKEN4_LSB                                           0
#define WDR_14_WDR_DIFF_DARKEN5_MASK                                     0xff00
#define WDR_14_WDR_DIFF_DARKEN5_LSB                                           8
#define WDR_14_WDR_DIFF_DARKEN6_MASK                                   0xff0000
#define WDR_14_WDR_DIFF_DARKEN6_LSB                                          16
#define WDR_14_WDR_DIFF_DARKEN7_MASK                                 0xff000000
#define WDR_14_WDR_DIFF_DARKEN7_LSB                                          24

/* ISP_WDR_15 */
#define WDR_15_WDR_DIFF_DARKEN8_MASK                                       0xff
#define WDR_15_WDR_DIFF_DARKEN8_LSB                                           0
#define WDR_15_WDR_DIFF_DARKEN9_MASK                                     0xff00
#define WDR_15_WDR_DIFF_DARKEN9_LSB                                           8
#define WDR_15_WDR_DIFF_DARKEN10_MASK                                  0xff0000
#define WDR_15_WDR_DIFF_DARKEN10_LSB                                         16
#define WDR_15_WDR_DIFF_DARKEN11_MASK                                0xff000000
#define WDR_15_WDR_DIFF_DARKEN11_LSB                                         24

/* ISP_WDR_16 */
#define WDR_16_WDR_DIFF_DARKEN12_MASK                                      0xff
#define WDR_16_WDR_DIFF_DARKEN12_LSB                                          0
#define WDR_16_WDR_DIFF_DARKEN13_MASK                                    0xff00
#define WDR_16_WDR_DIFF_DARKEN13_LSB                                          8
#define WDR_16_WDR_DIFF_DARKEN14_MASK                                  0xff0000
#define WDR_16_WDR_DIFF_DARKEN14_LSB                                         16
#define WDR_16_WDR_DIFF_DARKEN15_MASK                                0xff000000
#define WDR_16_WDR_DIFF_DARKEN15_LSB                                         24

/* ISP_WDR_17 */
#define WDR_17_WDR_DIFF_DARKEN16_MASK                                      0xff
#define WDR_17_WDR_DIFF_DARKEN16_LSB                                          0
#define WDR_17_WDR_DIFF_STR_MASK                                       0xff0000
#define WDR_17_WDR_DIFF_STR_LSB                                              16

/* ISP_WDR_18 */
#define WDR_18_WDR_DIFF_BRIGHTEN0_MASK                                     0xff
#define WDR_18_WDR_DIFF_BRIGHTEN0_LSB                                         0
#define WDR_18_WDR_DIFF_BRIGHTEN1_MASK                                   0xff00
#define WDR_18_WDR_DIFF_BRIGHTEN1_LSB                                         8
#define WDR_18_WDR_DIFF_BRIGHTEN2_MASK                                 0xff0000
#define WDR_18_WDR_DIFF_BRIGHTEN2_LSB                                        16
#define WDR_18_WDR_DIFF_BRIGHTEN3_MASK                               0xff000000
#define WDR_18_WDR_DIFF_BRIGHTEN3_LSB                                        24

/* ISP_WDR_19 */
#define WDR_19_WDR_DIFF_BRIGHTEN4_MASK                                     0xff
#define WDR_19_WDR_DIFF_BRIGHTEN4_LSB                                         0
#define WDR_19_WDR_DIFF_BRIGHTEN5_MASK                                   0xff00
#define WDR_19_WDR_DIFF_BRIGHTEN5_LSB                                         8
#define WDR_19_WDR_DIFF_BRIGHTEN6_MASK                                 0xff0000
#define WDR_19_WDR_DIFF_BRIGHTEN6_LSB                                        16
#define WDR_19_WDR_DIFF_BRIGHTEN7_MASK                               0xff000000
#define WDR_19_WDR_DIFF_BRIGHTEN7_LSB                                        24

/* ISP_WDR_20 */
#define WDR_20_WDR_DIFF_BRIGHTEN8_MASK                                     0xff
#define WDR_20_WDR_DIFF_BRIGHTEN8_LSB                                         0
#define WDR_20_WDR_DIFF_BRIGHTEN9_MASK                                   0xff00
#define WDR_20_WDR_DIFF_BRIGHTEN9_LSB                                         8
#define WDR_20_WDR_DIFF_BRIGHTEN10_MASK                                0xff0000
#define WDR_20_WDR_DIFF_BRIGHTEN10_LSB                                       16
#define WDR_20_WDR_DIFF_BRIGHTEN11_MASK                              0xff000000
#define WDR_20_WDR_DIFF_BRIGHTEN11_LSB                                       24

/* ISP_WDR_21 */
#define WDR_21_WDR_DIFF_BRIGHTEN12_MASK                                    0xff
#define WDR_21_WDR_DIFF_BRIGHTEN12_LSB                                        0
#define WDR_21_WDR_DIFF_BRIGHTEN13_MASK                                  0xff00
#define WDR_21_WDR_DIFF_BRIGHTEN13_LSB                                        8
#define WDR_21_WDR_DIFF_BRIGHTEN14_MASK                                0xff0000
#define WDR_21_WDR_DIFF_BRIGHTEN14_LSB                                       16
#define WDR_21_WDR_DIFF_BRIGHTEN15_MASK                              0xff000000
#define WDR_21_WDR_DIFF_BRIGHTEN15_LSB                                       24

/* ISP_WDR_22 */
#define WDR_22_WDR_DIFF_BRIGHTEN16_MASK                                    0xff
#define WDR_22_WDR_DIFF_BRIGHTEN16_LSB                                        0

/* ISP_WDR_23 */
#define WDR_23_WDR_COEFF0_MASK                                           0x3fff
#define WDR_23_WDR_COEFF0_LSB                                                 0
#define WDR_23_WDR_COEFF1_MASK                                       0x3fff0000
#define WDR_23_WDR_COEFF1_LSB                                                16

/* ISP_WDR_24 */
#define WDR_24_WDR_COEFF2_MASK                                           0x3fff
#define WDR_24_WDR_COEFF2_LSB                                                 0
#define WDR_24_WDR_COEFF3_MASK                                       0x3fff0000
#define WDR_24_WDR_COEFF3_LSB                                                16

/* ISP_WDR_25 */
#define WDR_25_WDR_CONTRAST_LUT0_MASK                                     0x1ff
#define WDR_25_WDR_CONTRAST_LUT0_LSB                                          0
#define WDR_25_WDR_CONTRAST_LUT1_MASK                                   0x3fe00
#define WDR_25_WDR_CONTRAST_LUT1_LSB                                          9
#define WDR_25_WDR_CONTRAST_LUT2_MASK                                 0x7fc0000
#define WDR_25_WDR_CONTRAST_LUT2_LSB                                         18

/* ISP_WDR_26 */
#define WDR_26_WDR_CONTRAST_LUT3_MASK                                     0x1ff
#define WDR_26_WDR_CONTRAST_LUT3_LSB                                          0
#define WDR_26_WDR_CONTRAST_LUT4_MASK                                   0x3fe00
#define WDR_26_WDR_CONTRAST_LUT4_LSB                                          9
#define WDR_26_WDR_CONTRAST_LUT5_MASK                                 0x7fc0000
#define WDR_26_WDR_CONTRAST_LUT5_LSB                                         18

/* ISP_WDR_27 */
#define WDR_27_WDR_CONTRAST_LUT6_MASK                                     0x1ff
#define WDR_27_WDR_CONTRAST_LUT6_LSB                                          0
#define WDR_27_WDR_CONTRAST_LUT7_MASK                                   0x3fe00
#define WDR_27_WDR_CONTRAST_LUT7_LSB                                          9
#define WDR_27_WDR_CONTRAST_LUT8_MASK                                 0x7fc0000
#define WDR_27_WDR_CONTRAST_LUT8_LSB                                         18

/* ISP_WDR_28 */
#define WDR_28_WDR_STR_MASK                                                0xff
#define WDR_28_WDR_STR_LSB                                                    0
#define WDR_28_WDR_MAXGAIN_MASK                                        0xff0000
#define WDR_28_WDR_MAXGAIN_LSB                                               16
#define WDR_28_WDR_MINGAIN_MASK                                      0xff000000
#define WDR_28_WDR_MINGAIN_LSB                                               24

/* ISP_WDR_29 */
#define WDR_29_WDR_OUTBLD_LUT0_MASK                                       0xfff
#define WDR_29_WDR_OUTBLD_LUT0_LSB                                            0
#define WDR_29_WDR_OUTBLD_LUT1_MASK                                   0xfff0000
#define WDR_29_WDR_OUTBLD_LUT1_LSB                                           16

/* ISP_WDR_30 */
#define WDR_30_WDR_OUTBLD_LUT2_MASK                                       0xfff
#define WDR_30_WDR_OUTBLD_LUT2_LSB                                            0
#define WDR_30_WDR_OUTBLD_LUT3_MASK                                   0xfff0000
#define WDR_30_WDR_OUTBLD_LUT3_LSB                                           16

/* ISP_WDR_31 */
#define WDR_31_WDR_OUTBLD_LUT4_MASK                                       0xfff
#define WDR_31_WDR_OUTBLD_LUT4_LSB                                            0
#define WDR_31_WDR_OUTBLD_LUT5_MASK                                   0xfff0000
#define WDR_31_WDR_OUTBLD_LUT5_LSB                                           16

/* ISP_WDR_32 */
#define WDR_32_WDR_OUTBLD_LUT6_MASK                                       0xfff
#define WDR_32_WDR_OUTBLD_LUT6_LSB                                            0
#define WDR_32_WDR_OUTBLD_LUT7_MASK                                   0xfff0000
#define WDR_32_WDR_OUTBLD_LUT7_LSB                                           16

/* ISP_WDR_33 */
#define WDR_33_WDR_OUTBLD_LUT8_MASK                                       0xfff
#define WDR_33_WDR_OUTBLD_LUT8_LSB                                            0
#define WDR_33_WDR_OUTBLD_LUT9_MASK                                   0xfff0000
#define WDR_33_WDR_OUTBLD_LUT9_LSB                                           16

/* ISP_WDR_34 */
#define WDR_34_WDR_OUTBLD_LUT10_MASK                                      0xfff
#define WDR_34_WDR_OUTBLD_LUT10_LSB                                           0
#define WDR_34_WDR_OUTBLD_LUT11_MASK                                  0xfff0000
#define WDR_34_WDR_OUTBLD_LUT11_LSB                                          16

/* ISP_WDR_35 */
#define WDR_35_WDR_OUTBLD_LUT12_MASK                                      0xfff
#define WDR_35_WDR_OUTBLD_LUT12_LSB                                           0
#define WDR_35_WDR_OUTBLD_LUT13_MASK                                  0xfff0000
#define WDR_35_WDR_OUTBLD_LUT13_LSB                                          16

/* ISP_WDR_36 */
#define WDR_36_WDR_OUTBLD_LUT14_MASK                                      0xfff
#define WDR_36_WDR_OUTBLD_LUT14_LSB                                           0
#define WDR_36_WDR_OUTBLD_LUT15_MASK                                  0xfff0000
#define WDR_36_WDR_OUTBLD_LUT15_LSB                                          16

/* ISP_WDR_37 */
#define WDR_37_WDR_OUTBLD_LUT16_MASK                                      0xfff
#define WDR_37_WDR_OUTBLD_LUT16_LSB                                           0
#define WDR_37_WDR_OUTBLD_LUT17_MASK                                  0xfff0000
#define WDR_37_WDR_OUTBLD_LUT17_LSB                                          16

/* ISP_WDR_38 */
#define WDR_38_WDR_OUTBLD_LUT18_MASK                                      0xfff
#define WDR_38_WDR_OUTBLD_LUT18_LSB                                           0
#define WDR_38_WDR_OUTBLD_LUT19_MASK                                  0xfff0000
#define WDR_38_WDR_OUTBLD_LUT19_LSB                                          16

/* ISP_WDR_39 */
#define WDR_39_WDR_OUTBLD_LUT20_MASK                                      0xfff
#define WDR_39_WDR_OUTBLD_LUT20_LSB                                           0
#define WDR_39_WDR_OUTBLD_LUT21_MASK                                  0xfff0000
#define WDR_39_WDR_OUTBLD_LUT21_LSB                                          16

/* ISP_WDR_40 */
#define WDR_40_WDR_OUTBLD_LUT22_MASK                                      0xfff
#define WDR_40_WDR_OUTBLD_LUT22_LSB                                           0
#define WDR_40_WDR_OUTBLD_LUT23_MASK                                  0xfff0000
#define WDR_40_WDR_OUTBLD_LUT23_LSB                                          16

/* ISP_WDR_41 */
#define WDR_41_WDR_OUTBLD_LUT24_MASK                                      0xfff
#define WDR_41_WDR_OUTBLD_LUT24_LSB                                           0
#define WDR_41_WDR_OUTBLD_LUT25_MASK                                  0xfff0000
#define WDR_41_WDR_OUTBLD_LUT25_LSB                                          16

/* ISP_WDR_42 */
#define WDR_42_WDR_OUTBLD_LUT26_MASK                                      0xfff
#define WDR_42_WDR_OUTBLD_LUT26_LSB                                           0
#define WDR_42_WDR_OUTBLD_LUT27_MASK                                  0xfff0000
#define WDR_42_WDR_OUTBLD_LUT27_LSB                                          16

/* ISP_WDR_43 */
#define WDR_43_WDR_OUTBLD_LUT28_MASK                                      0xfff
#define WDR_43_WDR_OUTBLD_LUT28_LSB                                           0
#define WDR_43_WDR_OUTBLD_LUT29_MASK                                  0xfff0000
#define WDR_43_WDR_OUTBLD_LUT29_LSB                                          16

/* ISP_WDR_44 */
#define WDR_44_WDR_OUTBLD_LUT30_MASK                                      0xfff
#define WDR_44_WDR_OUTBLD_LUT30_LSB                                           0
#define WDR_44_WDR_OUTBLD_LUT31_MASK                                  0xfff0000
#define WDR_44_WDR_OUTBLD_LUT31_LSB                                          16

/* ISP_WDR_45 */
#define WDR_45_WDR_OUTBLD_LUT32_MASK                                      0xfff
#define WDR_45_WDR_OUTBLD_LUT32_LSB                                           0
#define WDR_45_WDR_OUTBLD_LUT33_MASK                                  0xfff0000
#define WDR_45_WDR_OUTBLD_LUT33_LSB                                          16

/* ISP_WDR_46 */
#define WDR_46_WDR_OUTBLD_LUT34_MASK                                      0xfff
#define WDR_46_WDR_OUTBLD_LUT34_LSB                                           0
#define WDR_46_WDR_OUTBLD_LUT35_MASK                                  0xfff0000
#define WDR_46_WDR_OUTBLD_LUT35_LSB                                          16

/* ISP_WDR_47 */
#define WDR_47_WDR_OUTBLD_LUT36_MASK                                      0xfff
#define WDR_47_WDR_OUTBLD_LUT36_LSB                                           0
#define WDR_47_WDR_OUTBLD_LUT37_MASK                                  0xfff0000
#define WDR_47_WDR_OUTBLD_LUT37_LSB                                          16

/* ISP_WDR_48 */
#define WDR_48_WDR_OUTBLD_LUT38_MASK                                      0xfff
#define WDR_48_WDR_OUTBLD_LUT38_LSB                                           0
#define WDR_48_WDR_OUTBLD_LUT39_MASK                                  0xfff0000
#define WDR_48_WDR_OUTBLD_LUT39_LSB                                          16

/* ISP_WDR_49 */
#define WDR_49_WDR_OUTBLD_LUT40_MASK                                      0xfff
#define WDR_49_WDR_OUTBLD_LUT40_LSB                                           0
#define WDR_49_WDR_OUTBLD_LUT41_MASK                                  0xfff0000
#define WDR_49_WDR_OUTBLD_LUT41_LSB                                          16

/* ISP_WDR_50 */
#define WDR_50_WDR_OUTBLD_LUT42_MASK                                      0xfff
#define WDR_50_WDR_OUTBLD_LUT42_LSB                                           0
#define WDR_50_WDR_OUTBLD_LUT43_MASK                                  0xfff0000
#define WDR_50_WDR_OUTBLD_LUT43_LSB                                          16

/* ISP_WDR_51 */
#define WDR_51_WDR_OUTBLD_LUT44_MASK                                      0xfff
#define WDR_51_WDR_OUTBLD_LUT44_LSB                                           0
#define WDR_51_WDR_OUTBLD_LUT45_MASK                                  0xfff0000
#define WDR_51_WDR_OUTBLD_LUT45_LSB                                          16

/* ISP_WDR_52 */
#define WDR_52_WDR_OUTBLD_LUT46_MASK                                      0xfff
#define WDR_52_WDR_OUTBLD_LUT46_LSB                                           0
#define WDR_52_WDR_OUTBLD_LUT47_MASK                                  0xfff0000
#define WDR_52_WDR_OUTBLD_LUT47_LSB                                          16

/* ISP_WDR_53 */
#define WDR_53_WDR_OUTBLD_LUT48_MASK                                      0xfff
#define WDR_53_WDR_OUTBLD_LUT48_LSB                                           0
#define WDR_53_WDR_OUTBLD_LUT49_MASK                                  0xfff0000
#define WDR_53_WDR_OUTBLD_LUT49_LSB                                          16

/* ISP_WDR_54 */
#define WDR_54_WDR_OUTBLD_LUT50_MASK                                      0xfff
#define WDR_54_WDR_OUTBLD_LUT50_LSB                                           0
#define WDR_54_WDR_OUTBLD_LUT51_MASK                                  0xfff0000
#define WDR_54_WDR_OUTBLD_LUT51_LSB                                          16

/* ISP_WDR_55 */
#define WDR_55_WDR_OUTBLD_LUT52_MASK                                      0xfff
#define WDR_55_WDR_OUTBLD_LUT52_LSB                                           0
#define WDR_55_WDR_OUTBLD_LUT53_MASK                                  0xfff0000
#define WDR_55_WDR_OUTBLD_LUT53_LSB                                          16

/* ISP_WDR_56 */
#define WDR_56_WDR_OUTBLD_LUT54_MASK                                      0xfff
#define WDR_56_WDR_OUTBLD_LUT54_LSB                                           0
#define WDR_56_WDR_OUTBLD_LUT55_MASK                                  0xfff0000
#define WDR_56_WDR_OUTBLD_LUT55_LSB                                          16

/* ISP_WDR_57 */
#define WDR_57_WDR_OUTBLD_LUT56_MASK                                      0xfff
#define WDR_57_WDR_OUTBLD_LUT56_LSB                                           0
#define WDR_57_WDR_OUTBLD_LUT57_MASK                                  0xfff0000
#define WDR_57_WDR_OUTBLD_LUT57_LSB                                          16

/* ISP_WDR_58 */
#define WDR_58_WDR_OUTBLD_LUT58_MASK                                      0xfff
#define WDR_58_WDR_OUTBLD_LUT58_LSB                                           0
#define WDR_58_WDR_OUTBLD_LUT59_MASK                                  0xfff0000
#define WDR_58_WDR_OUTBLD_LUT59_LSB                                          16

/* ISP_WDR_59 */
#define WDR_59_WDR_OUTBLD_LUT60_MASK                                      0xfff
#define WDR_59_WDR_OUTBLD_LUT60_LSB                                           0
#define WDR_59_WDR_OUTBLD_LUT61_MASK                                  0xfff0000
#define WDR_59_WDR_OUTBLD_LUT61_LSB                                          16

/* ISP_WDR_60 */
#define WDR_60_WDR_OUTBLD_LUT62_MASK                                      0xfff
#define WDR_60_WDR_OUTBLD_LUT62_LSB                                           0
#define WDR_60_WDR_OUTBLD_LUT63_MASK                                  0xfff0000
#define WDR_60_WDR_OUTBLD_LUT63_LSB                                          16

/* ISP_WDR_61 */
#define WDR_61_WDR_OUTBLD_LUT64_MASK                                      0xfff
#define WDR_61_WDR_OUTBLD_LUT64_LSB                                           0
#define WDR_61_WDR_OUTBLD_LUT65_MASK                                  0xfff0000
#define WDR_61_WDR_OUTBLD_LUT65_LSB                                          16

/* ISP_WDR_62 */
#define WDR_62_WDR_OUTBLD_LUT66_MASK                                      0xfff
#define WDR_62_WDR_OUTBLD_LUT66_LSB                                           0
#define WDR_62_WDR_OUTBLD_LUT67_MASK                                  0xfff0000
#define WDR_62_WDR_OUTBLD_LUT67_LSB                                          16

/* ISP_WDR_63 */
#define WDR_63_WDR_OUTBLD_LUT68_MASK                                      0xfff
#define WDR_63_WDR_OUTBLD_LUT68_LSB                                           0
#define WDR_63_WDR_OUTBLD_LUT69_MASK                                  0xfff0000
#define WDR_63_WDR_OUTBLD_LUT69_LSB                                          16

/* ISP_WDR_64 */
#define WDR_64_WDR_OUTBLD_LUT70_MASK                                      0xfff
#define WDR_64_WDR_OUTBLD_LUT70_LSB                                           0
#define WDR_64_WDR_OUTBLD_LUT71_MASK                                  0xfff0000
#define WDR_64_WDR_OUTBLD_LUT71_LSB                                          16

/* ISP_WDR_65 */
#define WDR_65_WDR_OUTBLD_LUT72_MASK                                      0xfff
#define WDR_65_WDR_OUTBLD_LUT72_LSB                                           0
#define WDR_65_WDR_OUTBLD_LUT73_MASK                                  0xfff0000
#define WDR_65_WDR_OUTBLD_LUT73_LSB                                          16

/* ISP_WDR_66 */
#define WDR_66_WDR_OUTBLD_LUT74_MASK                                      0xfff
#define WDR_66_WDR_OUTBLD_LUT74_LSB                                           0
#define WDR_66_WDR_OUTBLD_LUT75_MASK                                  0xfff0000
#define WDR_66_WDR_OUTBLD_LUT75_LSB                                          16

/* ISP_WDR_67 */
#define WDR_67_WDR_OUTBLD_LUT76_MASK                                      0xfff
#define WDR_67_WDR_OUTBLD_LUT76_LSB                                           0
#define WDR_67_WDR_OUTBLD_LUT77_MASK                                  0xfff0000
#define WDR_67_WDR_OUTBLD_LUT77_LSB                                          16

/* ISP_WDR_68 */
#define WDR_68_WDR_OUTBLD_LUT78_MASK                                      0xfff
#define WDR_68_WDR_OUTBLD_LUT78_LSB                                           0
#define WDR_68_WDR_OUTBLD_LUT79_MASK                                  0xfff0000
#define WDR_68_WDR_OUTBLD_LUT79_LSB                                          16

/* ISP_WDR_69 */
#define WDR_69_WDR_OUTBLD_LUT80_MASK                                      0xfff
#define WDR_69_WDR_OUTBLD_LUT80_LSB                                           0
#define WDR_69_WDR_OUTBLD_LUT81_MASK                                  0xfff0000
#define WDR_69_WDR_OUTBLD_LUT81_LSB                                          16

/* ISP_WDR_70 */
#define WDR_70_WDR_OUTBLD_LUT82_MASK                                      0xfff
#define WDR_70_WDR_OUTBLD_LUT82_LSB                                           0
#define WDR_70_WDR_OUTBLD_LUT83_MASK                                  0xfff0000
#define WDR_70_WDR_OUTBLD_LUT83_LSB                                          16

/* ISP_WDR_71 */
#define WDR_71_WDR_OUTBLD_LUT84_MASK                                      0xfff
#define WDR_71_WDR_OUTBLD_LUT84_LSB                                           0
#define WDR_71_WDR_OUTBLD_LUT85_MASK                                  0xfff0000
#define WDR_71_WDR_OUTBLD_LUT85_LSB                                          16

/* ISP_WDR_72 */
#define WDR_72_WDR_OUTBLD_LUT86_MASK                                      0xfff
#define WDR_72_WDR_OUTBLD_LUT86_LSB                                           0
#define WDR_72_WDR_OUTBLD_LUT87_MASK                                  0xfff0000
#define WDR_72_WDR_OUTBLD_LUT87_LSB                                          16

/* ISP_WDR_73 */
#define WDR_73_WDR_OUTBLD_LUT88_MASK                                      0xfff
#define WDR_73_WDR_OUTBLD_LUT88_LSB                                           0
#define WDR_73_WDR_OUTBLD_LUT89_MASK                                  0xfff0000
#define WDR_73_WDR_OUTBLD_LUT89_LSB                                          16

/* ISP_WDR_74 */
#define WDR_74_WDR_OUTBLD_LUT90_MASK                                      0xfff
#define WDR_74_WDR_OUTBLD_LUT90_LSB                                           0
#define WDR_74_WDR_OUTBLD_LUT91_MASK                                  0xfff0000
#define WDR_74_WDR_OUTBLD_LUT91_LSB                                          16

/* ISP_WDR_75 */
#define WDR_75_WDR_OUTBLD_LUT92_MASK                                      0xfff
#define WDR_75_WDR_OUTBLD_LUT92_LSB                                           0
#define WDR_75_WDR_OUTBLD_LUT93_MASK                                  0xfff0000
#define WDR_75_WDR_OUTBLD_LUT93_LSB                                          16

/* ISP_WDR_76 */
#define WDR_76_WDR_OUTBLD_LUT94_MASK                                      0xfff
#define WDR_76_WDR_OUTBLD_LUT94_LSB                                           0
#define WDR_76_WDR_OUTBLD_LUT95_MASK                                  0xfff0000
#define WDR_76_WDR_OUTBLD_LUT95_LSB                                          16

/* ISP_WDR_77 */
#define WDR_77_WDR_OUTBLD_LUT96_MASK                                      0xfff
#define WDR_77_WDR_OUTBLD_LUT96_LSB                                           0
#define WDR_77_WDR_OUTBLD_LUT97_MASK                                  0xfff0000
#define WDR_77_WDR_OUTBLD_LUT97_LSB                                          16

/* ISP_WDR_78 */
#define WDR_78_WDR_OUTBLD_LUT98_MASK                                      0xfff
#define WDR_78_WDR_OUTBLD_LUT98_LSB                                           0
#define WDR_78_WDR_OUTBLD_LUT99_MASK                                  0xfff0000
#define WDR_78_WDR_OUTBLD_LUT99_LSB                                          16

/* ISP_WDR_79 */
#define WDR_79_WDR_OUTBLD_LUT100_MASK                                     0xfff
#define WDR_79_WDR_OUTBLD_LUT100_LSB                                          0
#define WDR_79_WDR_OUTBLD_LUT101_MASK                                 0xfff0000
#define WDR_79_WDR_OUTBLD_LUT101_LSB                                         16

/* ISP_WDR_80 */
#define WDR_80_WDR_OUTBLD_LUT102_MASK                                     0xfff
#define WDR_80_WDR_OUTBLD_LUT102_LSB                                          0
#define WDR_80_WDR_OUTBLD_LUT103_MASK                                 0xfff0000
#define WDR_80_WDR_OUTBLD_LUT103_LSB                                         16

/* ISP_WDR_81 */
#define WDR_81_WDR_OUTBLD_LUT104_MASK                                     0xfff
#define WDR_81_WDR_OUTBLD_LUT104_LSB                                          0
#define WDR_81_WDR_OUTBLD_LUT105_MASK                                 0xfff0000
#define WDR_81_WDR_OUTBLD_LUT105_LSB                                         16

/* ISP_WDR_82 */
#define WDR_82_WDR_OUTBLD_LUT106_MASK                                     0xfff
#define WDR_82_WDR_OUTBLD_LUT106_LSB                                          0
#define WDR_82_WDR_OUTBLD_LUT107_MASK                                 0xfff0000
#define WDR_82_WDR_OUTBLD_LUT107_LSB                                         16

/* ISP_WDR_83 */
#define WDR_83_WDR_OUTBLD_LUT108_MASK                                     0xfff
#define WDR_83_WDR_OUTBLD_LUT108_LSB                                          0
#define WDR_83_WDR_OUTBLD_LUT109_MASK                                 0xfff0000
#define WDR_83_WDR_OUTBLD_LUT109_LSB                                         16

/* ISP_WDR_84 */
#define WDR_84_WDR_OUTBLD_LUT110_MASK                                     0xfff
#define WDR_84_WDR_OUTBLD_LUT110_LSB                                          0
#define WDR_84_WDR_OUTBLD_LUT111_MASK                                 0xfff0000
#define WDR_84_WDR_OUTBLD_LUT111_LSB                                         16

/* ISP_WDR_85 */
#define WDR_85_WDR_OUTBLD_LUT112_MASK                                     0xfff
#define WDR_85_WDR_OUTBLD_LUT112_LSB                                          0
#define WDR_85_WDR_OUTBLD_LUT113_MASK                                 0xfff0000
#define WDR_85_WDR_OUTBLD_LUT113_LSB                                         16

/* ISP_WDR_86 */
#define WDR_86_WDR_OUTBLD_LUT114_MASK                                     0xfff
#define WDR_86_WDR_OUTBLD_LUT114_LSB                                          0
#define WDR_86_WDR_OUTBLD_LUT115_MASK                                 0xfff0000
#define WDR_86_WDR_OUTBLD_LUT115_LSB                                         16

/* ISP_WDR_87 */
#define WDR_87_WDR_OUTBLD_LUT116_MASK                                     0xfff
#define WDR_87_WDR_OUTBLD_LUT116_LSB                                          0
#define WDR_87_WDR_OUTBLD_LUT117_MASK                                 0xfff0000
#define WDR_87_WDR_OUTBLD_LUT117_LSB                                         16

/* ISP_WDR_88 */
#define WDR_88_WDR_OUTBLD_LUT118_MASK                                     0xfff
#define WDR_88_WDR_OUTBLD_LUT118_LSB                                          0
#define WDR_88_WDR_OUTBLD_LUT119_MASK                                 0xfff0000
#define WDR_88_WDR_OUTBLD_LUT119_LSB                                         16

/* ISP_WDR_89 */
#define WDR_89_WDR_OUTBLD_LUT120_MASK                                     0xfff
#define WDR_89_WDR_OUTBLD_LUT120_LSB                                          0
#define WDR_89_WDR_OUTBLD_LUT121_MASK                                 0xfff0000
#define WDR_89_WDR_OUTBLD_LUT121_LSB                                         16

/* ISP_WDR_90 */
#define WDR_90_WDR_OUTBLD_LUT122_MASK                                     0xfff
#define WDR_90_WDR_OUTBLD_LUT122_LSB                                          0
#define WDR_90_WDR_OUTBLD_LUT123_MASK                                 0xfff0000
#define WDR_90_WDR_OUTBLD_LUT123_LSB                                         16

/* ISP_WDR_91 */
#define WDR_91_WDR_OUTBLD_LUT124_MASK                                     0xfff
#define WDR_91_WDR_OUTBLD_LUT124_LSB                                          0
#define WDR_91_WDR_OUTBLD_LUT125_MASK                                 0xfff0000
#define WDR_91_WDR_OUTBLD_LUT125_LSB                                         16

/* ISP_WDR_92 */
#define WDR_92_WDR_OUTBLD_LUT126_MASK                                     0xfff
#define WDR_92_WDR_OUTBLD_LUT126_LSB                                          0
#define WDR_92_WDR_OUTBLD_LUT127_MASK                                 0xfff0000
#define WDR_92_WDR_OUTBLD_LUT127_LSB                                         16

/* ISP_WDR_93 */
#define WDR_93_WDR_OUTBLD_LUT128_MASK                                     0xfff
#define WDR_93_WDR_OUTBLD_LUT128_LSB                                          0

/* ISP_DM_0 */
#define DM_0_DM_CORG_EN_MASK                                                0x1
#define DM_0_DM_CORG_EN_LSB                                                   0
#define DM_0_DM_RATIO_CORG_MASK                                            0xf0
#define DM_0_DM_RATIO_CORG_LSB                                                4
#define DM_0_DM_CORRB_EN_MASK                                           0x10000
#define DM_0_DM_CORRB_EN_LSB                                                 16
#define DM_0_DM_RATIO_CORRB_MASK                                       0xf00000
#define DM_0_DM_RATIO_CORRB_LSB                                              20

/* ISP_DM_1 */
#define DM_1_DM_TH_EDGEDIFF_MASK                                          0xfff
#define DM_1_DM_TH_EDGEDIFF_LSB                                               0
#define DM_1_DM_TH_EDGESUM_MASK                                       0xfff0000
#define DM_1_DM_TH_EDGESUM_LSB                                               16

/* ISP_DM_2 */
#define DM_2_DM_HF_TH_EDGEHF_MASK                                         0xfff
#define DM_2_DM_HF_TH_EDGEHF_LSB                                              0
#define DM_2_DM_HF_STEP_EDGEHF_MASK                                   0x1ff0000
#define DM_2_DM_HF_STEP_EDGEHF_LSB                                           16

/* ISP_DM_3 */
#define DM_3_DM_HF_TH_EDGEDIFF_MASK                                       0xfff
#define DM_3_DM_HF_TH_EDGEDIFF_LSB                                            0
#define DM_3_DM_HF_STEP_EDGEDIFF_MASK                                 0x1ff0000
#define DM_3_DM_HF_STEP_EDGEDIFF_LSB                                         16

/* ISP_DM_4 */
#define DM_4_DM_HF_TH_EDGECROSSDIFF_MASK                                  0xfff
#define DM_4_DM_HF_TH_EDGECROSSDIFF_LSB                                       0
#define DM_4_DM_HF_STEP_EDGECROSSDIFF_MASK                            0x1ff0000
#define DM_4_DM_HF_STEP_EDGECROSSDIFF_LSB                                    16

/* ISP_DM_5 */
#define DM_5_DM_HF_TH_EDGECROSSMIN_MASK                                   0xfff
#define DM_5_DM_HF_TH_EDGECROSSMIN_LSB                                        0
#define DM_5_DM_HF_STEP_EDGECROSSMIN_MASK                             0x1ff0000
#define DM_5_DM_HF_STEP_EDGECROSSMIN_LSB                                     16

/* ISP_DM_6 */
#define DM_6_DM_HF_TH_COLORDIFF_MASK                                      0xfff
#define DM_6_DM_HF_TH_COLORDIFF_LSB                                           0
#define DM_6_DM_HF_STEP_COLORDIFF_MASK                                0x1ff0000
#define DM_6_DM_HF_STEP_COLORDIFF_LSB                                        16

/* ISP_DM_7 */
#define DM_7_DM_FCS_STR_MASK                                               0xff
#define DM_7_DM_FCS_STR_LSB                                                   0

/* ISP_DM_8 */
#define DM_8_DM_FCS_STR_EDGE_LUT0_MASK                                     0x3f
#define DM_8_DM_FCS_STR_EDGE_LUT0_LSB                                         0
#define DM_8_DM_FCS_STR_EDGE_LUT1_MASK                                   0x3f00
#define DM_8_DM_FCS_STR_EDGE_LUT1_LSB                                         8
#define DM_8_DM_FCS_STR_EDGE_LUT2_MASK                                 0x3f0000
#define DM_8_DM_FCS_STR_EDGE_LUT2_LSB                                        16
#define DM_8_DM_FCS_STR_EDGE_LUT3_MASK                               0x3f000000
#define DM_8_DM_FCS_STR_EDGE_LUT3_LSB                                        24

/* ISP_DM_9 */
#define DM_9_DM_FCS_STR_EDGE_LUT4_MASK                                     0x3f
#define DM_9_DM_FCS_STR_EDGE_LUT4_LSB                                         0
#define DM_9_DM_FCS_STR_EDGE_LUT5_MASK                                   0x3f00
#define DM_9_DM_FCS_STR_EDGE_LUT5_LSB                                         8
#define DM_9_DM_FCS_STR_EDGE_LUT6_MASK                                 0x3f0000
#define DM_9_DM_FCS_STR_EDGE_LUT6_LSB                                        16
#define DM_9_DM_FCS_STR_EDGE_LUT7_MASK                               0x3f000000
#define DM_9_DM_FCS_STR_EDGE_LUT7_LSB                                        24

/* ISP_DM_10 */
#define DM_10_DM_FCS_STR_EDGE_LUT8_MASK                                    0x3f
#define DM_10_DM_FCS_STR_EDGE_LUT8_LSB                                        0
#define DM_10_DM_FCS_STR_EDGE_LUT9_MASK                                  0x3f00
#define DM_10_DM_FCS_STR_EDGE_LUT9_LSB                                        8
#define DM_10_DM_FCS_STR_EDGE_LUT10_MASK                               0x3f0000
#define DM_10_DM_FCS_STR_EDGE_LUT10_LSB                                      16
#define DM_10_DM_FCS_STR_EDGE_LUT11_MASK                             0x3f000000
#define DM_10_DM_FCS_STR_EDGE_LUT11_LSB                                      24

/* ISP_DM_11 */
#define DM_11_DM_FCS_STR_EDGE_LUT12_MASK                                   0x3f
#define DM_11_DM_FCS_STR_EDGE_LUT12_LSB                                       0
#define DM_11_DM_FCS_STR_EDGE_LUT13_MASK                                 0x3f00
#define DM_11_DM_FCS_STR_EDGE_LUT13_LSB                                       8
#define DM_11_DM_FCS_STR_EDGE_LUT14_MASK                               0x3f0000
#define DM_11_DM_FCS_STR_EDGE_LUT14_LSB                                      16
#define DM_11_DM_FCS_STR_EDGE_LUT15_MASK                             0x3f000000
#define DM_11_DM_FCS_STR_EDGE_LUT15_LSB                                      24

/* ISP_DM_12 */
#define DM_12_DM_FCS_STR_EDGE_LUT16_MASK                                   0x3f
#define DM_12_DM_FCS_STR_EDGE_LUT16_LSB                                       0

/* ISP_DM_13 */
#define DM_13_DM_SP_STR_GLOBAL_MASK                                        0xff
#define DM_13_DM_SP_STR_GLOBAL_LSB                                            0

/* ISP_DM_14 */
#define DM_14_DM_SP_STR_OVERSHOOT_MASK                                     0xff
#define DM_14_DM_SP_STR_OVERSHOOT_LSB                                         0
#define DM_14_DM_SP_STR_UNDERSHOOT_MASK                                0xff0000
#define DM_14_DM_SP_STR_UNDERSHOOT_LSB                                       16

/* ISP_DM_15 */
#define DM_15_DM_SP_GAIN_CTRL_MASK                                        0x1ff
#define DM_15_DM_SP_GAIN_CTRL_LSB                                             0
#define DM_15_DM_SP_DELTA_CLAMP_MASK                                  0xfff0000
#define DM_15_DM_SP_DELTA_CLAMP_LSB                                          16

/* ISP_DM_16 */
#define DM_16_DM_SP_STR_AC_LUT0_MASK                                       0xff
#define DM_16_DM_SP_STR_AC_LUT0_LSB                                           0
#define DM_16_DM_SP_STR_AC_LUT1_MASK                                     0xff00
#define DM_16_DM_SP_STR_AC_LUT1_LSB                                           8
#define DM_16_DM_SP_STR_AC_LUT2_MASK                                   0xff0000
#define DM_16_DM_SP_STR_AC_LUT2_LSB                                          16
#define DM_16_DM_SP_STR_AC_LUT3_MASK                                 0xff000000
#define DM_16_DM_SP_STR_AC_LUT3_LSB                                          24

/* ISP_DM_17 */
#define DM_17_DM_SP_STR_AC_LUT4_MASK                                       0xff
#define DM_17_DM_SP_STR_AC_LUT4_LSB                                           0
#define DM_17_DM_SP_STR_AC_LUT5_MASK                                     0xff00
#define DM_17_DM_SP_STR_AC_LUT5_LSB                                           8
#define DM_17_DM_SP_STR_AC_LUT6_MASK                                   0xff0000
#define DM_17_DM_SP_STR_AC_LUT6_LSB                                          16
#define DM_17_DM_SP_STR_AC_LUT7_MASK                                 0xff000000
#define DM_17_DM_SP_STR_AC_LUT7_LSB                                          24

/* ISP_DM_18 */
#define DM_18_DM_SP_STR_AC_LUT8_MASK                                       0xff
#define DM_18_DM_SP_STR_AC_LUT8_LSB                                           0
#define DM_18_DM_SP_STR_AC_LUT9_MASK                                     0xff00
#define DM_18_DM_SP_STR_AC_LUT9_LSB                                           8
#define DM_18_DM_SP_STR_AC_LUT10_MASK                                  0xff0000
#define DM_18_DM_SP_STR_AC_LUT10_LSB                                         16
#define DM_18_DM_SP_STR_AC_LUT11_MASK                                0xff000000
#define DM_18_DM_SP_STR_AC_LUT11_LSB                                         24

/* ISP_DM_19 */
#define DM_19_DM_SP_STR_AC_LUT12_MASK                                      0xff
#define DM_19_DM_SP_STR_AC_LUT12_LSB                                          0
#define DM_19_DM_SP_STR_AC_LUT13_MASK                                    0xff00
#define DM_19_DM_SP_STR_AC_LUT13_LSB                                          8
#define DM_19_DM_SP_STR_AC_LUT14_MASK                                  0xff0000
#define DM_19_DM_SP_STR_AC_LUT14_LSB                                         16
#define DM_19_DM_SP_STR_AC_LUT15_MASK                                0xff000000
#define DM_19_DM_SP_STR_AC_LUT15_LSB                                         24

/* ISP_DM_20 */
#define DM_20_DM_SP_STR_AC_LUT16_MASK                                      0xff
#define DM_20_DM_SP_STR_AC_LUT16_LSB                                          0
#define DM_20_DM_SP_STR_AC_GAIN_MASK                                   0xff0000
#define DM_20_DM_SP_STR_AC_GAIN_LSB                                          16

/* ISP_DM_21 */
#define DM_21_DM_SP_STR_EDGE_LUT0_MASK                                     0x7f
#define DM_21_DM_SP_STR_EDGE_LUT0_LSB                                         0
#define DM_21_DM_SP_STR_EDGE_LUT1_MASK                                   0x7f00
#define DM_21_DM_SP_STR_EDGE_LUT1_LSB                                         8
#define DM_21_DM_SP_STR_EDGE_LUT2_MASK                                 0x7f0000
#define DM_21_DM_SP_STR_EDGE_LUT2_LSB                                        16
#define DM_21_DM_SP_STR_EDGE_LUT3_MASK                               0x7f000000
#define DM_21_DM_SP_STR_EDGE_LUT3_LSB                                        24

/* ISP_DM_22 */
#define DM_22_DM_SP_STR_EDGE_LUT4_MASK                                     0x7f
#define DM_22_DM_SP_STR_EDGE_LUT4_LSB                                         0
#define DM_22_DM_SP_STR_EDGE_LUT5_MASK                                   0x7f00
#define DM_22_DM_SP_STR_EDGE_LUT5_LSB                                         8
#define DM_22_DM_SP_STR_EDGE_LUT6_MASK                                 0x7f0000
#define DM_22_DM_SP_STR_EDGE_LUT6_LSB                                        16
#define DM_22_DM_SP_STR_EDGE_LUT7_MASK                               0x7f000000
#define DM_22_DM_SP_STR_EDGE_LUT7_LSB                                        24

/* ISP_DM_23 */
#define DM_23_DM_SP_STR_EDGE_LUT8_MASK                                     0x7f
#define DM_23_DM_SP_STR_EDGE_LUT8_LSB                                         0
#define DM_23_DM_SP_STR_EDGE_LUT9_MASK                                   0x7f00
#define DM_23_DM_SP_STR_EDGE_LUT9_LSB                                         8
#define DM_23_DM_SP_STR_EDGE_LUT10_MASK                                0x7f0000
#define DM_23_DM_SP_STR_EDGE_LUT10_LSB                                       16
#define DM_23_DM_SP_STR_EDGE_LUT11_MASK                              0x7f000000
#define DM_23_DM_SP_STR_EDGE_LUT11_LSB                                       24

/* ISP_DM_24 */
#define DM_24_DM_SP_STR_EDGE_LUT12_MASK                                    0x7f
#define DM_24_DM_SP_STR_EDGE_LUT12_LSB                                        0
#define DM_24_DM_SP_STR_EDGE_LUT13_MASK                                  0x7f00
#define DM_24_DM_SP_STR_EDGE_LUT13_LSB                                        8
#define DM_24_DM_SP_STR_EDGE_LUT14_MASK                                0x7f0000
#define DM_24_DM_SP_STR_EDGE_LUT14_LSB                                       16
#define DM_24_DM_SP_STR_EDGE_LUT15_MASK                              0x7f000000
#define DM_24_DM_SP_STR_EDGE_LUT15_LSB                                       24

/* ISP_DM_25 */
#define DM_25_DM_SP_STR_EDGE_LUT16_MASK                                    0x7f
#define DM_25_DM_SP_STR_EDGE_LUT16_LSB                                        0

/* ISP_DM_26 */
#define DM_26_DM_SP_STR_LUMA_LUT0_MASK                                     0x7f
#define DM_26_DM_SP_STR_LUMA_LUT0_LSB                                         0
#define DM_26_DM_SP_STR_LUMA_LUT1_MASK                                   0x7f00
#define DM_26_DM_SP_STR_LUMA_LUT1_LSB                                         8
#define DM_26_DM_SP_STR_LUMA_LUT2_MASK                                 0x7f0000
#define DM_26_DM_SP_STR_LUMA_LUT2_LSB                                        16
#define DM_26_DM_SP_STR_LUMA_LUT3_MASK                               0x7f000000
#define DM_26_DM_SP_STR_LUMA_LUT3_LSB                                        24

/* ISP_DM_27 */
#define DM_27_DM_SP_STR_LUMA_LUT4_MASK                                     0x7f
#define DM_27_DM_SP_STR_LUMA_LUT4_LSB                                         0
#define DM_27_DM_SP_STR_LUMA_LUT5_MASK                                   0x7f00
#define DM_27_DM_SP_STR_LUMA_LUT5_LSB                                         8
#define DM_27_DM_SP_STR_LUMA_LUT6_MASK                                 0x7f0000
#define DM_27_DM_SP_STR_LUMA_LUT6_LSB                                        16
#define DM_27_DM_SP_STR_LUMA_LUT7_MASK                               0x7f000000
#define DM_27_DM_SP_STR_LUMA_LUT7_LSB                                        24

/* ISP_DM_28 */
#define DM_28_DM_SP_STR_LUMA_LUT8_MASK                                     0x7f
#define DM_28_DM_SP_STR_LUMA_LUT8_LSB                                         0
#define DM_28_DM_SP_STR_LUMA_LUT9_MASK                                   0x7f00
#define DM_28_DM_SP_STR_LUMA_LUT9_LSB                                         8
#define DM_28_DM_SP_STR_LUMA_LUT10_MASK                                0x7f0000
#define DM_28_DM_SP_STR_LUMA_LUT10_LSB                                       16
#define DM_28_DM_SP_STR_LUMA_LUT11_MASK                              0x7f000000
#define DM_28_DM_SP_STR_LUMA_LUT11_LSB                                       24

/* ISP_DM_29 */
#define DM_29_DM_SP_STR_LUMA_LUT12_MASK                                    0x7f
#define DM_29_DM_SP_STR_LUMA_LUT12_LSB                                        0
#define DM_29_DM_SP_STR_LUMA_LUT13_MASK                                  0x7f00
#define DM_29_DM_SP_STR_LUMA_LUT13_LSB                                        8
#define DM_29_DM_SP_STR_LUMA_LUT14_MASK                                0x7f0000
#define DM_29_DM_SP_STR_LUMA_LUT14_LSB                                       16
#define DM_29_DM_SP_STR_LUMA_LUT15_MASK                              0x7f000000
#define DM_29_DM_SP_STR_LUMA_LUT15_LSB                                       24

/* ISP_DM_30 */
#define DM_30_DM_SP_STR_LUMA_LUT16_MASK                                    0x7f
#define DM_30_DM_SP_STR_LUMA_LUT16_LSB                                        0

/* ISP_CCM_0 */
#define CCM_0_CCM_COEFF_RR_MASK                                           0xfff
#define CCM_0_CCM_COEFF_RR_LSB                                                0
#define CCM_0_CCM_COEFF_RG_MASK                                       0xfff0000
#define CCM_0_CCM_COEFF_RG_LSB                                               16

/* ISP_CCM_1 */
#define CCM_1_CCM_COEFF_RB_MASK                                           0xfff
#define CCM_1_CCM_COEFF_RB_LSB                                                0
#define CCM_1_CCM_COEFF_GR_MASK                                       0xfff0000
#define CCM_1_CCM_COEFF_GR_LSB                                               16

/* ISP_CCM_2 */
#define CCM_2_CCM_COEFF_GG_MASK                                           0xfff
#define CCM_2_CCM_COEFF_GG_LSB                                                0
#define CCM_2_CCM_COEFF_GB_MASK                                       0xfff0000
#define CCM_2_CCM_COEFF_GB_LSB                                               16

/* ISP_CCM_3 */
#define CCM_3_CCM_COEFF_BR_MASK                                           0xfff
#define CCM_3_CCM_COEFF_BR_LSB                                                0
#define CCM_3_CCM_COEFF_BG_MASK                                       0xfff0000
#define CCM_3_CCM_COEFF_BG_LSB                                               16

/* ISP_CCM_4 */
#define CCM_4_CCM_COEFF_BB_MASK                                           0xfff
#define CCM_4_CCM_COEFF_BB_LSB                                                0

/* ISP_CCM_5 */
#define CCM_5_CCM_STR_SAT_LUT0_MASK                                        0x3f
#define CCM_5_CCM_STR_SAT_LUT0_LSB                                            0
#define CCM_5_CCM_STR_SAT_LUT1_MASK                                      0x3f00
#define CCM_5_CCM_STR_SAT_LUT1_LSB                                            8
#define CCM_5_CCM_STR_SAT_LUT2_MASK                                    0x3f0000
#define CCM_5_CCM_STR_SAT_LUT2_LSB                                           16
#define CCM_5_CCM_STR_SAT_LUT3_MASK                                  0x3f000000
#define CCM_5_CCM_STR_SAT_LUT3_LSB                                           24

/* ISP_CCM_6 */
#define CCM_6_CCM_STR_SAT_LUT4_MASK                                        0x3f
#define CCM_6_CCM_STR_SAT_LUT4_LSB                                            0
#define CCM_6_CCM_STR_SAT_LUT5_MASK                                      0x3f00
#define CCM_6_CCM_STR_SAT_LUT5_LSB                                            8
#define CCM_6_CCM_STR_SAT_LUT6_MASK                                    0x3f0000
#define CCM_6_CCM_STR_SAT_LUT6_LSB                                           16
#define CCM_6_CCM_STR_SAT_LUT7_MASK                                  0x3f000000
#define CCM_6_CCM_STR_SAT_LUT7_LSB                                           24

/* ISP_CCM_7 */
#define CCM_7_CCM_STR_SAT_LUT8_MASK                                        0x3f
#define CCM_7_CCM_STR_SAT_LUT8_LSB                                            0
#define CCM_7_CCM_STR_SAT_LUT9_MASK                                      0x3f00
#define CCM_7_CCM_STR_SAT_LUT9_LSB                                            8
#define CCM_7_CCM_STR_SAT_LUT10_MASK                                   0x3f0000
#define CCM_7_CCM_STR_SAT_LUT10_LSB                                          16
#define CCM_7_CCM_STR_SAT_LUT11_MASK                                 0x3f000000
#define CCM_7_CCM_STR_SAT_LUT11_LSB                                          24

/* ISP_CCM_8 */
#define CCM_8_CCM_STR_SAT_LUT12_MASK                                       0x3f
#define CCM_8_CCM_STR_SAT_LUT12_LSB                                           0
#define CCM_8_CCM_STR_SAT_LUT13_MASK                                     0x3f00
#define CCM_8_CCM_STR_SAT_LUT13_LSB                                           8
#define CCM_8_CCM_STR_SAT_LUT14_MASK                                   0x3f0000
#define CCM_8_CCM_STR_SAT_LUT14_LSB                                          16
#define CCM_8_CCM_STR_SAT_LUT15_MASK                                 0x3f000000
#define CCM_8_CCM_STR_SAT_LUT15_LSB                                          24

/* ISP_CCM_9 */
#define CCM_9_CCM_STR_SAT_LUT16_MASK                                       0x3f
#define CCM_9_CCM_STR_SAT_LUT16_LSB                                           0

/* ISP_CCM_10 */
#define CCM_10_CCM_STR_LUM_LUT0_MASK                                       0x3f
#define CCM_10_CCM_STR_LUM_LUT0_LSB                                           0
#define CCM_10_CCM_STR_LUM_LUT1_MASK                                     0x3f00
#define CCM_10_CCM_STR_LUM_LUT1_LSB                                           8
#define CCM_10_CCM_STR_LUM_LUT2_MASK                                   0x3f0000
#define CCM_10_CCM_STR_LUM_LUT2_LSB                                          16
#define CCM_10_CCM_STR_LUM_LUT3_MASK                                 0x3f000000
#define CCM_10_CCM_STR_LUM_LUT3_LSB                                          24

/* ISP_CCM_11 */
#define CCM_11_CCM_STR_LUM_LUT4_MASK                                       0x3f
#define CCM_11_CCM_STR_LUM_LUT4_LSB                                           0
#define CCM_11_CCM_STR_LUM_LUT5_MASK                                     0x3f00
#define CCM_11_CCM_STR_LUM_LUT5_LSB                                           8
#define CCM_11_CCM_STR_LUM_LUT6_MASK                                   0x3f0000
#define CCM_11_CCM_STR_LUM_LUT6_LSB                                          16
#define CCM_11_CCM_STR_LUM_LUT7_MASK                                 0x3f000000
#define CCM_11_CCM_STR_LUM_LUT7_LSB                                          24

/* ISP_CCM_12 */
#define CCM_12_CCM_STR_LUM_LUT8_MASK                                       0x3f
#define CCM_12_CCM_STR_LUM_LUT8_LSB                                           0
#define CCM_12_CCM_STR_LUM_LUT9_MASK                                     0x3f00
#define CCM_12_CCM_STR_LUM_LUT9_LSB                                           8
#define CCM_12_CCM_STR_LUM_LUT10_MASK                                  0x3f0000
#define CCM_12_CCM_STR_LUM_LUT10_LSB                                         16
#define CCM_12_CCM_STR_LUM_LUT11_MASK                                0x3f000000
#define CCM_12_CCM_STR_LUM_LUT11_LSB                                         24

/* ISP_CCM_13 */
#define CCM_13_CCM_STR_LUM_LUT12_MASK                                      0x3f
#define CCM_13_CCM_STR_LUM_LUT12_LSB                                          0
#define CCM_13_CCM_STR_LUM_LUT13_MASK                                    0x3f00
#define CCM_13_CCM_STR_LUM_LUT13_LSB                                          8
#define CCM_13_CCM_STR_LUM_LUT14_MASK                                  0x3f0000
#define CCM_13_CCM_STR_LUM_LUT14_LSB                                         16
#define CCM_13_CCM_STR_LUM_LUT15_MASK                                0x3f000000
#define CCM_13_CCM_STR_LUM_LUT15_LSB                                         24

/* ISP_CCM_14 */
#define CCM_14_CCM_STR_LUM_LUT16_MASK                                      0x3f
#define CCM_14_CCM_STR_LUM_LUT16_LSB                                          0

/* ISP_GAMMA_0 */
#define GAMMA_0_GAMMA_LUT0_MASK                                           0x3ff
#define GAMMA_0_GAMMA_LUT0_LSB                                                0
#define GAMMA_0_GAMMA_LUT1_MASK                                         0xffc00
#define GAMMA_0_GAMMA_LUT1_LSB                                               10
#define GAMMA_0_GAMMA_LUT2_MASK                                      0x3ff00000
#define GAMMA_0_GAMMA_LUT2_LSB                                               20

/* ISP_GAMMA_1 */
#define GAMMA_1_GAMMA_LUT3_MASK                                           0x3ff
#define GAMMA_1_GAMMA_LUT3_LSB                                                0
#define GAMMA_1_GAMMA_LUT4_MASK                                         0xffc00
#define GAMMA_1_GAMMA_LUT4_LSB                                               10
#define GAMMA_1_GAMMA_LUT5_MASK                                      0x3ff00000
#define GAMMA_1_GAMMA_LUT5_LSB                                               20

/* ISP_GAMMA_2 */
#define GAMMA_2_GAMMA_LUT6_MASK                                           0x3ff
#define GAMMA_2_GAMMA_LUT6_LSB                                                0
#define GAMMA_2_GAMMA_LUT7_MASK                                         0xffc00
#define GAMMA_2_GAMMA_LUT7_LSB                                               10
#define GAMMA_2_GAMMA_LUT8_MASK                                      0x3ff00000
#define GAMMA_2_GAMMA_LUT8_LSB                                               20

/* ISP_GAMMA_3 */
#define GAMMA_3_GAMMA_LUT9_MASK                                           0x3ff
#define GAMMA_3_GAMMA_LUT9_LSB                                                0
#define GAMMA_3_GAMMA_LUT10_MASK                                        0xffc00
#define GAMMA_3_GAMMA_LUT10_LSB                                              10
#define GAMMA_3_GAMMA_LUT11_MASK                                     0x3ff00000
#define GAMMA_3_GAMMA_LUT11_LSB                                              20

/* ISP_GAMMA_4 */
#define GAMMA_4_GAMMA_LUT12_MASK                                          0x3ff
#define GAMMA_4_GAMMA_LUT12_LSB                                               0
#define GAMMA_4_GAMMA_LUT13_MASK                                        0xffc00
#define GAMMA_4_GAMMA_LUT13_LSB                                              10
#define GAMMA_4_GAMMA_LUT14_MASK                                     0x3ff00000
#define GAMMA_4_GAMMA_LUT14_LSB                                              20

/* ISP_GAMMA_5 */
#define GAMMA_5_GAMMA_LUT15_MASK                                          0x3ff
#define GAMMA_5_GAMMA_LUT15_LSB                                               0
#define GAMMA_5_GAMMA_LUT16_MASK                                        0xffc00
#define GAMMA_5_GAMMA_LUT16_LSB                                              10
#define GAMMA_5_GAMMA_LUT17_MASK                                     0x3ff00000
#define GAMMA_5_GAMMA_LUT17_LSB                                              20

/* ISP_GAMMA_6 */
#define GAMMA_6_GAMMA_LUT18_MASK                                          0x3ff
#define GAMMA_6_GAMMA_LUT18_LSB                                               0
#define GAMMA_6_GAMMA_LUT19_MASK                                        0xffc00
#define GAMMA_6_GAMMA_LUT19_LSB                                              10
#define GAMMA_6_GAMMA_LUT20_MASK                                     0x3ff00000
#define GAMMA_6_GAMMA_LUT20_LSB                                              20

/* ISP_GAMMA_7 */
#define GAMMA_7_GAMMA_LUT21_MASK                                          0x3ff
#define GAMMA_7_GAMMA_LUT21_LSB                                               0
#define GAMMA_7_GAMMA_LUT22_MASK                                        0xffc00
#define GAMMA_7_GAMMA_LUT22_LSB                                              10
#define GAMMA_7_GAMMA_LUT23_MASK                                     0x3ff00000
#define GAMMA_7_GAMMA_LUT23_LSB                                              20

/* ISP_GAMMA_8 */
#define GAMMA_8_GAMMA_LUT24_MASK                                          0x3ff
#define GAMMA_8_GAMMA_LUT24_LSB                                               0
#define GAMMA_8_GAMMA_LUT25_MASK                                        0xffc00
#define GAMMA_8_GAMMA_LUT25_LSB                                              10
#define GAMMA_8_GAMMA_LUT26_MASK                                     0x3ff00000
#define GAMMA_8_GAMMA_LUT26_LSB                                              20

/* ISP_GAMMA_9 */
#define GAMMA_9_GAMMA_LUT27_MASK                                          0x3ff
#define GAMMA_9_GAMMA_LUT27_LSB                                               0
#define GAMMA_9_GAMMA_LUT28_MASK                                        0xffc00
#define GAMMA_9_GAMMA_LUT28_LSB                                              10
#define GAMMA_9_GAMMA_LUT29_MASK                                     0x3ff00000
#define GAMMA_9_GAMMA_LUT29_LSB                                              20

/* ISP_GAMMA_10 */
#define GAMMA_10_GAMMA_LUT30_MASK                                         0x3ff
#define GAMMA_10_GAMMA_LUT30_LSB                                              0
#define GAMMA_10_GAMMA_LUT31_MASK                                       0xffc00
#define GAMMA_10_GAMMA_LUT31_LSB                                             10
#define GAMMA_10_GAMMA_LUT32_MASK                                    0x3ff00000
#define GAMMA_10_GAMMA_LUT32_LSB                                             20

/* ISP_GAMMA_11 */
#define GAMMA_11_GAMMA_LUT33_MASK                                         0x3ff
#define GAMMA_11_GAMMA_LUT33_LSB                                              0
#define GAMMA_11_GAMMA_LUT34_MASK                                       0xffc00
#define GAMMA_11_GAMMA_LUT34_LSB                                             10
#define GAMMA_11_GAMMA_LUT35_MASK                                    0x3ff00000
#define GAMMA_11_GAMMA_LUT35_LSB                                             20

/* ISP_GAMMA_12 */
#define GAMMA_12_GAMMA_LUT36_MASK                                         0x3ff
#define GAMMA_12_GAMMA_LUT36_LSB                                              0
#define GAMMA_12_GAMMA_LUT37_MASK                                       0xffc00
#define GAMMA_12_GAMMA_LUT37_LSB                                             10
#define GAMMA_12_GAMMA_LUT38_MASK                                    0x3ff00000
#define GAMMA_12_GAMMA_LUT38_LSB                                             20

/* ISP_GAMMA_13 */
#define GAMMA_13_GAMMA_LUT39_MASK                                         0x3ff
#define GAMMA_13_GAMMA_LUT39_LSB                                              0
#define GAMMA_13_GAMMA_LUT40_MASK                                       0xffc00
#define GAMMA_13_GAMMA_LUT40_LSB                                             10
#define GAMMA_13_GAMMA_LUT41_MASK                                    0x3ff00000
#define GAMMA_13_GAMMA_LUT41_LSB                                             20

/* ISP_GAMMA_14 */
#define GAMMA_14_GAMMA_LUT42_MASK                                         0x3ff
#define GAMMA_14_GAMMA_LUT42_LSB                                              0
#define GAMMA_14_GAMMA_LUT43_MASK                                       0xffc00
#define GAMMA_14_GAMMA_LUT43_LSB                                             10
#define GAMMA_14_GAMMA_LUT44_MASK                                    0x3ff00000
#define GAMMA_14_GAMMA_LUT44_LSB                                             20

/* ISP_GAMMA_15 */
#define GAMMA_15_GAMMA_LUT45_MASK                                         0x3ff
#define GAMMA_15_GAMMA_LUT45_LSB                                              0
#define GAMMA_15_GAMMA_LUT46_MASK                                       0xffc00
#define GAMMA_15_GAMMA_LUT46_LSB                                             10
#define GAMMA_15_GAMMA_LUT47_MASK                                    0x3ff00000
#define GAMMA_15_GAMMA_LUT47_LSB                                             20

/* ISP_GAMMA_16 */
#define GAMMA_16_GAMMA_LUT48_MASK                                         0x3ff
#define GAMMA_16_GAMMA_LUT48_LSB                                              0
#define GAMMA_16_GAMMA_LUT49_MASK                                       0xffc00
#define GAMMA_16_GAMMA_LUT49_LSB                                             10
#define GAMMA_16_GAMMA_LUT50_MASK                                    0x3ff00000
#define GAMMA_16_GAMMA_LUT50_LSB                                             20

/* ISP_GAMMA_17 */
#define GAMMA_17_GAMMA_LUT51_MASK                                         0x3ff
#define GAMMA_17_GAMMA_LUT51_LSB                                              0
#define GAMMA_17_GAMMA_LUT52_MASK                                       0xffc00
#define GAMMA_17_GAMMA_LUT52_LSB                                             10
#define GAMMA_17_GAMMA_LUT53_MASK                                    0x3ff00000
#define GAMMA_17_GAMMA_LUT53_LSB                                             20

/* ISP_GAMMA_18 */
#define GAMMA_18_GAMMA_LUT54_MASK                                         0x3ff
#define GAMMA_18_GAMMA_LUT54_LSB                                              0
#define GAMMA_18_GAMMA_LUT55_MASK                                       0xffc00
#define GAMMA_18_GAMMA_LUT55_LSB                                             10
#define GAMMA_18_GAMMA_LUT56_MASK                                    0x3ff00000
#define GAMMA_18_GAMMA_LUT56_LSB                                             20

/* ISP_GAMMA_19 */
#define GAMMA_19_GAMMA_LUT57_MASK                                         0x3ff
#define GAMMA_19_GAMMA_LUT57_LSB                                              0
#define GAMMA_19_GAMMA_LUT58_MASK                                       0xffc00
#define GAMMA_19_GAMMA_LUT58_LSB                                             10
#define GAMMA_19_GAMMA_LUT59_MASK                                    0x3ff00000
#define GAMMA_19_GAMMA_LUT59_LSB                                             20

/* ISP_GAMMA_20 */
#define GAMMA_20_GAMMA_LUT60_MASK                                         0x3ff
#define GAMMA_20_GAMMA_LUT60_LSB                                              0
#define GAMMA_20_GAMMA_LUT61_MASK                                       0xffc00
#define GAMMA_20_GAMMA_LUT61_LSB                                             10
#define GAMMA_20_GAMMA_LUT62_MASK                                    0x3ff00000
#define GAMMA_20_GAMMA_LUT62_LSB                                             20

/* ISP_GAMMA_21 */
#define GAMMA_21_GAMMA_LUT63_MASK                                         0x3ff
#define GAMMA_21_GAMMA_LUT63_LSB                                              0
#define GAMMA_21_GAMMA_LUT64_MASK                                       0xffc00
#define GAMMA_21_GAMMA_LUT64_LSB                                             10
#define GAMMA_21_GAMMA_LUT65_MASK                                    0x3ff00000
#define GAMMA_21_GAMMA_LUT65_LSB                                             20

/* ISP_GAMMA_22 */
#define GAMMA_22_GAMMA_LUT66_MASK                                         0x3ff
#define GAMMA_22_GAMMA_LUT66_LSB                                              0
#define GAMMA_22_GAMMA_LUT67_MASK                                       0xffc00
#define GAMMA_22_GAMMA_LUT67_LSB                                             10
#define GAMMA_22_GAMMA_LUT68_MASK                                    0x3ff00000
#define GAMMA_22_GAMMA_LUT68_LSB                                             20

/* ISP_GAMMA_23 */
#define GAMMA_23_GAMMA_LUT69_MASK                                         0x3ff
#define GAMMA_23_GAMMA_LUT69_LSB                                              0
#define GAMMA_23_GAMMA_LUT70_MASK                                       0xffc00
#define GAMMA_23_GAMMA_LUT70_LSB                                             10
#define GAMMA_23_GAMMA_LUT71_MASK                                    0x3ff00000
#define GAMMA_23_GAMMA_LUT71_LSB                                             20

/* ISP_GAMMA_24 */
#define GAMMA_24_GAMMA_LUT72_MASK                                         0x3ff
#define GAMMA_24_GAMMA_LUT72_LSB                                              0
#define GAMMA_24_GAMMA_LUT73_MASK                                       0xffc00
#define GAMMA_24_GAMMA_LUT73_LSB                                             10
#define GAMMA_24_GAMMA_LUT74_MASK                                    0x3ff00000
#define GAMMA_24_GAMMA_LUT74_LSB                                             20

/* ISP_GAMMA_25 */
#define GAMMA_25_GAMMA_LUT75_MASK                                         0x3ff
#define GAMMA_25_GAMMA_LUT75_LSB                                              0
#define GAMMA_25_GAMMA_LUT76_MASK                                       0xffc00
#define GAMMA_25_GAMMA_LUT76_LSB                                             10
#define GAMMA_25_GAMMA_LUT77_MASK                                    0x3ff00000
#define GAMMA_25_GAMMA_LUT77_LSB                                             20

/* ISP_GAMMA_26 */
#define GAMMA_26_GAMMA_LUT78_MASK                                         0x3ff
#define GAMMA_26_GAMMA_LUT78_LSB                                              0
#define GAMMA_26_GAMMA_LUT79_MASK                                       0xffc00
#define GAMMA_26_GAMMA_LUT79_LSB                                             10
#define GAMMA_26_GAMMA_LUT80_MASK                                    0x3ff00000
#define GAMMA_26_GAMMA_LUT80_LSB                                             20

/* ISP_GAMMA_27 */
#define GAMMA_27_GAMMA_LUT81_MASK                                         0x3ff
#define GAMMA_27_GAMMA_LUT81_LSB                                              0
#define GAMMA_27_GAMMA_LUT82_MASK                                       0xffc00
#define GAMMA_27_GAMMA_LUT82_LSB                                             10
#define GAMMA_27_GAMMA_LUT83_MASK                                    0x3ff00000
#define GAMMA_27_GAMMA_LUT83_LSB                                             20

/* ISP_GAMMA_28 */
#define GAMMA_28_GAMMA_LUT84_MASK                                         0x3ff
#define GAMMA_28_GAMMA_LUT84_LSB                                              0
#define GAMMA_28_GAMMA_LUT85_MASK                                       0xffc00
#define GAMMA_28_GAMMA_LUT85_LSB                                             10
#define GAMMA_28_GAMMA_LUT86_MASK                                    0x3ff00000
#define GAMMA_28_GAMMA_LUT86_LSB                                             20

/* ISP_GAMMA_29 */
#define GAMMA_29_GAMMA_LUT87_MASK                                         0x3ff
#define GAMMA_29_GAMMA_LUT87_LSB                                              0
#define GAMMA_29_GAMMA_LUT88_MASK                                       0xffc00
#define GAMMA_29_GAMMA_LUT88_LSB                                             10
#define GAMMA_29_GAMMA_LUT89_MASK                                    0x3ff00000
#define GAMMA_29_GAMMA_LUT89_LSB                                             20

/* ISP_GAMMA_30 */
#define GAMMA_30_GAMMA_LUT90_MASK                                         0x3ff
#define GAMMA_30_GAMMA_LUT90_LSB                                              0
#define GAMMA_30_GAMMA_LUT91_MASK                                       0xffc00
#define GAMMA_30_GAMMA_LUT91_LSB                                             10
#define GAMMA_30_GAMMA_LUT92_MASK                                    0x3ff00000
#define GAMMA_30_GAMMA_LUT92_LSB                                             20

/* ISP_GAMMA_31 */
#define GAMMA_31_GAMMA_LUT93_MASK                                         0x3ff
#define GAMMA_31_GAMMA_LUT93_LSB                                              0
#define GAMMA_31_GAMMA_LUT94_MASK                                       0xffc00
#define GAMMA_31_GAMMA_LUT94_LSB                                             10
#define GAMMA_31_GAMMA_LUT95_MASK                                    0x3ff00000
#define GAMMA_31_GAMMA_LUT95_LSB                                             20

/* ISP_GAMMA_32 */
#define GAMMA_32_GAMMA_LUT96_MASK                                         0x3ff
#define GAMMA_32_GAMMA_LUT96_LSB                                              0
#define GAMMA_32_GAMMA_LUT97_MASK                                       0xffc00
#define GAMMA_32_GAMMA_LUT97_LSB                                             10
#define GAMMA_32_GAMMA_LUT98_MASK                                    0x3ff00000
#define GAMMA_32_GAMMA_LUT98_LSB                                             20

/* ISP_GAMMA_33 */
#define GAMMA_33_GAMMA_LUT99_MASK                                         0x3ff
#define GAMMA_33_GAMMA_LUT99_LSB                                              0
#define GAMMA_33_GAMMA_LUT100_MASK                                      0xffc00
#define GAMMA_33_GAMMA_LUT100_LSB                                            10
#define GAMMA_33_GAMMA_LUT101_MASK                                   0x3ff00000
#define GAMMA_33_GAMMA_LUT101_LSB                                            20

/* ISP_GAMMA_34 */
#define GAMMA_34_GAMMA_LUT102_MASK                                        0x3ff
#define GAMMA_34_GAMMA_LUT102_LSB                                             0
#define GAMMA_34_GAMMA_LUT103_MASK                                      0xffc00
#define GAMMA_34_GAMMA_LUT103_LSB                                            10
#define GAMMA_34_GAMMA_LUT104_MASK                                   0x3ff00000
#define GAMMA_34_GAMMA_LUT104_LSB                                            20

/* ISP_GAMMA_35 */
#define GAMMA_35_GAMMA_LUT105_MASK                                        0x3ff
#define GAMMA_35_GAMMA_LUT105_LSB                                             0
#define GAMMA_35_GAMMA_LUT106_MASK                                      0xffc00
#define GAMMA_35_GAMMA_LUT106_LSB                                            10
#define GAMMA_35_GAMMA_LUT107_MASK                                   0x3ff00000
#define GAMMA_35_GAMMA_LUT107_LSB                                            20

/* ISP_GAMMA_36 */
#define GAMMA_36_GAMMA_LUT108_MASK                                        0x3ff
#define GAMMA_36_GAMMA_LUT108_LSB                                             0
#define GAMMA_36_GAMMA_LUT109_MASK                                      0xffc00
#define GAMMA_36_GAMMA_LUT109_LSB                                            10
#define GAMMA_36_GAMMA_LUT110_MASK                                   0x3ff00000
#define GAMMA_36_GAMMA_LUT110_LSB                                            20

/* ISP_GAMMA_37 */
#define GAMMA_37_GAMMA_LUT111_MASK                                        0x3ff
#define GAMMA_37_GAMMA_LUT111_LSB                                             0
#define GAMMA_37_GAMMA_LUT112_MASK                                      0xffc00
#define GAMMA_37_GAMMA_LUT112_LSB                                            10
#define GAMMA_37_GAMMA_LUT113_MASK                                   0x3ff00000
#define GAMMA_37_GAMMA_LUT113_LSB                                            20

/* ISP_GAMMA_38 */
#define GAMMA_38_GAMMA_LUT114_MASK                                        0x3ff
#define GAMMA_38_GAMMA_LUT114_LSB                                             0
#define GAMMA_38_GAMMA_LUT115_MASK                                      0xffc00
#define GAMMA_38_GAMMA_LUT115_LSB                                            10
#define GAMMA_38_GAMMA_LUT116_MASK                                   0x3ff00000
#define GAMMA_38_GAMMA_LUT116_LSB                                            20

/* ISP_GAMMA_39 */
#define GAMMA_39_GAMMA_LUT117_MASK                                        0x3ff
#define GAMMA_39_GAMMA_LUT117_LSB                                             0
#define GAMMA_39_GAMMA_LUT118_MASK                                      0xffc00
#define GAMMA_39_GAMMA_LUT118_LSB                                            10
#define GAMMA_39_GAMMA_LUT119_MASK                                   0x3ff00000
#define GAMMA_39_GAMMA_LUT119_LSB                                            20

/* ISP_GAMMA_40 */
#define GAMMA_40_GAMMA_LUT120_MASK                                        0x3ff
#define GAMMA_40_GAMMA_LUT120_LSB                                             0
#define GAMMA_40_GAMMA_LUT121_MASK                                      0xffc00
#define GAMMA_40_GAMMA_LUT121_LSB                                            10
#define GAMMA_40_GAMMA_LUT122_MASK                                   0x3ff00000
#define GAMMA_40_GAMMA_LUT122_LSB                                            20

/* ISP_GAMMA_41 */
#define GAMMA_41_GAMMA_LUT123_MASK                                        0x3ff
#define GAMMA_41_GAMMA_LUT123_LSB                                             0
#define GAMMA_41_GAMMA_LUT124_MASK                                      0xffc00
#define GAMMA_41_GAMMA_LUT124_LSB                                            10
#define GAMMA_41_GAMMA_LUT125_MASK                                   0x3ff00000
#define GAMMA_41_GAMMA_LUT125_LSB                                            20

/* ISP_GAMMA_42 */
#define GAMMA_42_GAMMA_LUT126_MASK                                        0x3ff
#define GAMMA_42_GAMMA_LUT126_LSB                                             0
#define GAMMA_42_GAMMA_LUT127_MASK                                      0xffc00
#define GAMMA_42_GAMMA_LUT127_LSB                                            10
#define GAMMA_42_GAMMA_LUT128_MASK                                   0x3ff00000
#define GAMMA_42_GAMMA_LUT128_LSB                                            20

/* ISP_CST_0 */
#define CST_0_CST_COEFF_YR_MASK                                           0xfff
#define CST_0_CST_COEFF_YR_LSB                                                0
#define CST_0_CST_COEFF_YG_MASK                                       0xfff0000
#define CST_0_CST_COEFF_YG_LSB                                               16

/* ISP_CST_1 */
#define CST_1_CST_COEFF_YB_MASK                                           0xfff
#define CST_1_CST_COEFF_YB_LSB                                                0
#define CST_1_CST_COEFF_UR_MASK                                       0xfff0000
#define CST_1_CST_COEFF_UR_LSB                                               16

/* ISP_CST_2 */
#define CST_2_CST_COEFF_UG_MASK                                           0xfff
#define CST_2_CST_COEFF_UG_LSB                                                0
#define CST_2_CST_COEFF_UB_MASK                                       0xfff0000
#define CST_2_CST_COEFF_UB_LSB                                               16

/* ISP_CST_3 */
#define CST_3_CST_COEFF_VR_MASK                                           0xfff
#define CST_3_CST_COEFF_VR_LSB                                                0
#define CST_3_CST_COEFF_VG_MASK                                       0xfff0000
#define CST_3_CST_COEFF_VG_LSB                                               16

/* ISP_CST_4 */
#define CST_4_CST_COEFF_VB_MASK                                           0xfff
#define CST_4_CST_COEFF_VB_LSB                                                0

/* ISP_CST_5 */
#define CST_5_CST_OFS_Y_MASK                                               0xff
#define CST_5_CST_OFS_Y_LSB                                                   0
#define CST_5_CST_OFS_U_MASK                                             0xff00
#define CST_5_CST_OFS_U_LSB                                                   8
#define CST_5_CST_OFS_V_MASK                                           0xff0000
#define CST_5_CST_OFS_V_LSB                                                  16

/* ISP_CST_6 */
#define CST_6_CST_Y_CONTRAST_MASK                                          0xff
#define CST_6_CST_Y_CONTRAST_LSB                                              0
#define CST_6_CST_C_SATURATION_MASK                                    0xff0000
#define CST_6_CST_C_SATURATION_LSB                                           16

/* ISP_CST_7 */
#define CST_7_CST_SATURATION_LUT0_MASK                                     0x3f
#define CST_7_CST_SATURATION_LUT0_LSB                                         0
#define CST_7_CST_SATURATION_LUT1_MASK                                   0x3f00
#define CST_7_CST_SATURATION_LUT1_LSB                                         8
#define CST_7_CST_SATURATION_LUT2_MASK                                 0x3f0000
#define CST_7_CST_SATURATION_LUT2_LSB                                        16
#define CST_7_CST_SATURATION_LUT3_MASK                               0x3f000000
#define CST_7_CST_SATURATION_LUT3_LSB                                        24

/* ISP_CST_8 */
#define CST_8_CST_SATURATION_LUT4_MASK                                     0x3f
#define CST_8_CST_SATURATION_LUT4_LSB                                         0
#define CST_8_CST_SATURATION_LUT5_MASK                                   0x3f00
#define CST_8_CST_SATURATION_LUT5_LSB                                         8
#define CST_8_CST_SATURATION_LUT6_MASK                                 0x3f0000
#define CST_8_CST_SATURATION_LUT6_LSB                                        16
#define CST_8_CST_SATURATION_LUT7_MASK                               0x3f000000
#define CST_8_CST_SATURATION_LUT7_LSB                                        24

/* ISP_CST_9 */
#define CST_9_CST_SATURATION_LUT8_MASK                                     0x3f
#define CST_9_CST_SATURATION_LUT8_LSB                                         0
#define CST_9_CST_SATURATION_LUT9_MASK                                   0x3f00
#define CST_9_CST_SATURATION_LUT9_LSB                                         8
#define CST_9_CST_SATURATION_LUT10_MASK                                0x3f0000
#define CST_9_CST_SATURATION_LUT10_LSB                                       16
#define CST_9_CST_SATURATION_LUT11_MASK                              0x3f000000
#define CST_9_CST_SATURATION_LUT11_LSB                                       24

/* ISP_CST_10 */
#define CST_10_CST_SATURATION_LUT12_MASK                                   0x3f
#define CST_10_CST_SATURATION_LUT12_LSB                                       0
#define CST_10_CST_SATURATION_LUT13_MASK                                 0x3f00
#define CST_10_CST_SATURATION_LUT13_LSB                                       8
#define CST_10_CST_SATURATION_LUT14_MASK                               0x3f0000
#define CST_10_CST_SATURATION_LUT14_LSB                                      16
#define CST_10_CST_SATURATION_LUT15_MASK                             0x3f000000
#define CST_10_CST_SATURATION_LUT15_LSB                                      24

/* ISP_CST_11 */
#define CST_11_CST_SATURATION_LUT16_MASK                                   0x3f
#define CST_11_CST_SATURATION_LUT16_LSB                                       0

/* ISP_LCE_0 */
#define LCE_0_LCE_SUB_WIDTH_MASK                                           0x3f
#define LCE_0_LCE_SUB_WIDTH_LSB                                               0
#define LCE_0_LCE_SUB_HEIGHT_MASK                                      0x3f0000
#define LCE_0_LCE_SUB_HEIGHT_LSB                                             16

/* ISP_LCE_1 */
#define LCE_1_LCE_BLK_STCS_FACT_H_MASK                                 0xffffff
#define LCE_1_LCE_BLK_STCS_FACT_H_LSB                                         0

/* ISP_LCE_2 */
#define LCE_2_LCE_BLK_STCS_FACT_V_MASK                                 0xffffff
#define LCE_2_LCE_BLK_STCS_FACT_V_LSB                                         0

/* ISP_LCE_3 */
#define LCE_3_LCE_SCALING_FACT_H_MASK                                    0xffff
#define LCE_3_LCE_SCALING_FACT_H_LSB                                          0
#define LCE_3_LCE_SCALING_FACT_V_MASK                                0xffff0000
#define LCE_3_LCE_SCALING_FACT_V_LSB                                         16

/* ISP_LCE_4 */
#define LCE_4_LCE_BLK_SIZE_H_MASK                                         0x3ff
#define LCE_4_LCE_BLK_SIZE_H_LSB                                              0
#define LCE_4_LCE_BLK_SIZE_V_MASK                                     0x3ff0000
#define LCE_4_LCE_BLK_SIZE_V_LSB                                             16

/* ISP_LCE_5 */
#define LCE_5_LCE_BLK_SIZE_NORM_DIV_MASK                                0xfffff
#define LCE_5_LCE_BLK_SIZE_NORM_DIV_LSB                                       0

/* ISP_LCE_6 */
#define LCE_6_LCE_SUBIN_FILT_W0_MASK                                        0xf
#define LCE_6_LCE_SUBIN_FILT_W0_LSB                                           0
#define LCE_6_LCE_SUBIN_FILT_W1_MASK                                       0xf0
#define LCE_6_LCE_SUBIN_FILT_W1_LSB                                           4
#define LCE_6_LCE_SUBIN_FILT_W2_MASK                                      0xf00
#define LCE_6_LCE_SUBIN_FILT_W2_LSB                                           8
#define LCE_6_LCE_SUBIN_FILT_NORM_DIV_MASK                            0xfff0000
#define LCE_6_LCE_SUBIN_FILT_NORM_DIV_LSB                                    16

/* ISP_LCE_7 */
#define LCE_7_LCE_WT_DIFF_AVG_MASK                                         0x7f
#define LCE_7_LCE_WT_DIFF_AVG_LSB                                             0
#define LCE_7_LCE_WT_DIFF_POS_MASK                                       0x7f00
#define LCE_7_LCE_WT_DIFF_POS_LSB                                             8
#define LCE_7_LCE_WT_DIFF_NEG_MASK                                     0x7f0000
#define LCE_7_LCE_WT_DIFF_NEG_LSB                                            16

/* ISP_LCE_8 */
#define LCE_8_LCE_WT_LUMA_LUT0_MASK                                        0x3f
#define LCE_8_LCE_WT_LUMA_LUT0_LSB                                            0
#define LCE_8_LCE_WT_LUMA_LUT1_MASK                                      0x3f00
#define LCE_8_LCE_WT_LUMA_LUT1_LSB                                            8
#define LCE_8_LCE_WT_LUMA_LUT2_MASK                                    0x3f0000
#define LCE_8_LCE_WT_LUMA_LUT2_LSB                                           16
#define LCE_8_LCE_WT_LUMA_LUT3_MASK                                  0x3f000000
#define LCE_8_LCE_WT_LUMA_LUT3_LSB                                           24

/* ISP_LCE_9 */
#define LCE_9_LCE_WT_LUMA_LUT4_MASK                                        0x3f
#define LCE_9_LCE_WT_LUMA_LUT4_LSB                                            0
#define LCE_9_LCE_WT_LUMA_LUT5_MASK                                      0x3f00
#define LCE_9_LCE_WT_LUMA_LUT5_LSB                                            8
#define LCE_9_LCE_WT_LUMA_LUT6_MASK                                    0x3f0000
#define LCE_9_LCE_WT_LUMA_LUT6_LSB                                           16
#define LCE_9_LCE_WT_LUMA_LUT7_MASK                                  0x3f000000
#define LCE_9_LCE_WT_LUMA_LUT7_LSB                                           24

/* ISP_LCE_10 */
#define LCE_10_LCE_WT_LUMA_LUT8_MASK                                       0x3f
#define LCE_10_LCE_WT_LUMA_LUT8_LSB                                           0

/* ISP_CNR_0 */
#define CNR_0_CNR_SUB_WIDTH_MASK                                          0x3ff
#define CNR_0_CNR_SUB_WIDTH_LSB                                               0
#define CNR_0_CNR_SUB_HEIGHT_MASK                                     0x3ff0000
#define CNR_0_CNR_SUB_HEIGHT_LSB                                             16

/* ISP_CNR_1 */
#define CNR_1_CNR_BLK_STCS_FACT_H_MASK                                 0xffffff
#define CNR_1_CNR_BLK_STCS_FACT_H_LSB                                         0

/* ISP_CNR_2 */
#define CNR_2_CNR_BLK_STCS_FACT_V_MASK                                 0xffffff
#define CNR_2_CNR_BLK_STCS_FACT_V_LSB                                         0

/* ISP_CNR_3 */
#define CNR_3_CNR_SCALING_FACT_H_MASK                                    0xffff
#define CNR_3_CNR_SCALING_FACT_H_LSB                                          0
#define CNR_3_CNR_SCALING_FACT_V_MASK                                0xffff0000
#define CNR_3_CNR_SCALING_FACT_V_LSB                                         16

/* ISP_CNR_4 */
#define CNR_4_CNR_BLK_SIZE_H_MASK                                         0x3ff
#define CNR_4_CNR_BLK_SIZE_H_LSB                                              0
#define CNR_4_CNR_BLK_SIZE_V_MASK                                     0x3ff0000
#define CNR_4_CNR_BLK_SIZE_V_LSB                                             16

/* ISP_CNR_5 */
#define CNR_5_CNR_BLK_SIZE_NORM_DIV_MASK                                0xfffff
#define CNR_5_CNR_BLK_SIZE_NORM_DIV_LSB                                       0

/* ISP_CNR_6 */
#define CNR_6_CNR_STR_MASK                                                 0xff
#define CNR_6_CNR_STR_LSB                                                     0
#define CNR_6_CNR_DIFF_SHIFT_MASK                                       0x70000
#define CNR_6_CNR_DIFF_SHIFT_LSB                                             16

/* ISP_CNR_7 */
#define CNR_7_CNR_WT_SAT_THRESHOLD_MASK                                    0x7f
#define CNR_7_CNR_WT_SAT_THRESHOLD_LSB                                        0
#define CNR_7_CNR_WT_SAT_SLOPE_MASK                                    0xff0000
#define CNR_7_CNR_WT_SAT_SLOPE_LSB                                           16

/* ISP_CNR_8 */
#define CNR_8_CNR_WT_DIFF_LUT0_MASK                                        0x3f
#define CNR_8_CNR_WT_DIFF_LUT0_LSB                                            0
#define CNR_8_CNR_WT_DIFF_LUT1_MASK                                      0x3f00
#define CNR_8_CNR_WT_DIFF_LUT1_LSB                                            8
#define CNR_8_CNR_WT_DIFF_LUT2_MASK                                    0x3f0000
#define CNR_8_CNR_WT_DIFF_LUT2_LSB                                           16
#define CNR_8_CNR_WT_DIFF_LUT3_MASK                                  0x3f000000
#define CNR_8_CNR_WT_DIFF_LUT3_LSB                                           24

/* ISP_CNR_9 */
#define CNR_9_CNR_WT_DIFF_LUT4_MASK                                        0x3f
#define CNR_9_CNR_WT_DIFF_LUT4_LSB                                            0
#define CNR_9_CNR_WT_DIFF_LUT5_MASK                                      0x3f00
#define CNR_9_CNR_WT_DIFF_LUT5_LSB                                            8
#define CNR_9_CNR_WT_DIFF_LUT6_MASK                                    0x3f0000
#define CNR_9_CNR_WT_DIFF_LUT6_LSB                                           16
#define CNR_9_CNR_WT_DIFF_LUT7_MASK                                  0x3f000000
#define CNR_9_CNR_WT_DIFF_LUT7_LSB                                           24

/* ISP_CNR_10 */
#define CNR_10_CNR_WT_DIFF_LUT8_MASK                                       0x3f
#define CNR_10_CNR_WT_DIFF_LUT8_LSB                                           0
#define CNR_10_CNR_WT_DIFF_LUT9_MASK                                     0x3f00
#define CNR_10_CNR_WT_DIFF_LUT9_LSB                                           8
#define CNR_10_CNR_WT_DIFF_LUT10_MASK                                  0x3f0000
#define CNR_10_CNR_WT_DIFF_LUT10_LSB                                         16
#define CNR_10_CNR_WT_DIFF_LUT11_MASK                                0x3f000000
#define CNR_10_CNR_WT_DIFF_LUT11_LSB                                         24

/* ISP_CNR_11 */
#define CNR_11_CNR_WT_DIFF_LUT12_MASK                                      0x3f
#define CNR_11_CNR_WT_DIFF_LUT12_LSB                                          0
#define CNR_11_CNR_WT_DIFF_LUT13_MASK                                    0x3f00
#define CNR_11_CNR_WT_DIFF_LUT13_LSB                                          8
#define CNR_11_CNR_WT_DIFF_LUT14_MASK                                  0x3f0000
#define CNR_11_CNR_WT_DIFF_LUT14_LSB                                         16
#define CNR_11_CNR_WT_DIFF_LUT15_MASK                                0x3f000000
#define CNR_11_CNR_WT_DIFF_LUT15_LSB                                         24

/* ISP_CNR_12 */
#define CNR_12_CNR_WT_DIFF_LUT16_MASK                                      0x3f
#define CNR_12_CNR_WT_DIFF_LUT16_LSB                                          0

/* ISP_YCURVE_0 */
#define YCURVE_0_YCURVE_LUT0_MASK                                          0xff
#define YCURVE_0_YCURVE_LUT0_LSB                                              0
#define YCURVE_0_YCURVE_LUT1_MASK                                        0xff00
#define YCURVE_0_YCURVE_LUT1_LSB                                              8
#define YCURVE_0_YCURVE_LUT2_MASK                                      0xff0000
#define YCURVE_0_YCURVE_LUT2_LSB                                             16
#define YCURVE_0_YCURVE_LUT3_MASK                                    0xff000000
#define YCURVE_0_YCURVE_LUT3_LSB                                             24

/* ISP_YCURVE_1 */
#define YCURVE_1_YCURVE_LUT4_MASK                                          0xff
#define YCURVE_1_YCURVE_LUT4_LSB                                              0
#define YCURVE_1_YCURVE_LUT5_MASK                                        0xff00
#define YCURVE_1_YCURVE_LUT5_LSB                                              8
#define YCURVE_1_YCURVE_LUT6_MASK                                      0xff0000
#define YCURVE_1_YCURVE_LUT6_LSB                                             16
#define YCURVE_1_YCURVE_LUT7_MASK                                    0xff000000
#define YCURVE_1_YCURVE_LUT7_LSB                                             24

/* ISP_YCURVE_2 */
#define YCURVE_2_YCURVE_LUT8_MASK                                          0xff
#define YCURVE_2_YCURVE_LUT8_LSB                                              0
#define YCURVE_2_YCURVE_LUT9_MASK                                        0xff00
#define YCURVE_2_YCURVE_LUT9_LSB                                              8
#define YCURVE_2_YCURVE_LUT10_MASK                                     0xff0000
#define YCURVE_2_YCURVE_LUT10_LSB                                            16
#define YCURVE_2_YCURVE_LUT11_MASK                                   0xff000000
#define YCURVE_2_YCURVE_LUT11_LSB                                            24

/* ISP_YCURVE_3 */
#define YCURVE_3_YCURVE_LUT12_MASK                                         0xff
#define YCURVE_3_YCURVE_LUT12_LSB                                             0
#define YCURVE_3_YCURVE_LUT13_MASK                                       0xff00
#define YCURVE_3_YCURVE_LUT13_LSB                                             8
#define YCURVE_3_YCURVE_LUT14_MASK                                     0xff0000
#define YCURVE_3_YCURVE_LUT14_LSB                                            16
#define YCURVE_3_YCURVE_LUT15_MASK                                   0xff000000
#define YCURVE_3_YCURVE_LUT15_LSB                                            24

/* ISP_YCURVE_4 */
#define YCURVE_4_YCURVE_LUT16_MASK                                         0xff
#define YCURVE_4_YCURVE_LUT16_LSB                                             0
#define YCURVE_4_YCURVE_LUT17_MASK                                       0xff00
#define YCURVE_4_YCURVE_LUT17_LSB                                             8
#define YCURVE_4_YCURVE_LUT18_MASK                                     0xff0000
#define YCURVE_4_YCURVE_LUT18_LSB                                            16
#define YCURVE_4_YCURVE_LUT19_MASK                                   0xff000000
#define YCURVE_4_YCURVE_LUT19_LSB                                            24

/* ISP_YCURVE_5 */
#define YCURVE_5_YCURVE_LUT20_MASK                                         0xff
#define YCURVE_5_YCURVE_LUT20_LSB                                             0
#define YCURVE_5_YCURVE_LUT21_MASK                                       0xff00
#define YCURVE_5_YCURVE_LUT21_LSB                                             8
#define YCURVE_5_YCURVE_LUT22_MASK                                     0xff0000
#define YCURVE_5_YCURVE_LUT22_LSB                                            16
#define YCURVE_5_YCURVE_LUT23_MASK                                   0xff000000
#define YCURVE_5_YCURVE_LUT23_LSB                                            24

/* ISP_YCURVE_6 */
#define YCURVE_6_YCURVE_LUT24_MASK                                         0xff
#define YCURVE_6_YCURVE_LUT24_LSB                                             0
#define YCURVE_6_YCURVE_LUT25_MASK                                       0xff00
#define YCURVE_6_YCURVE_LUT25_LSB                                             8
#define YCURVE_6_YCURVE_LUT26_MASK                                     0xff0000
#define YCURVE_6_YCURVE_LUT26_LSB                                            16
#define YCURVE_6_YCURVE_LUT27_MASK                                   0xff000000
#define YCURVE_6_YCURVE_LUT27_LSB                                            24

/* ISP_YCURVE_7 */
#define YCURVE_7_YCURVE_LUT28_MASK                                         0xff
#define YCURVE_7_YCURVE_LUT28_LSB                                             0
#define YCURVE_7_YCURVE_LUT29_MASK                                       0xff00
#define YCURVE_7_YCURVE_LUT29_LSB                                             8
#define YCURVE_7_YCURVE_LUT30_MASK                                     0xff0000
#define YCURVE_7_YCURVE_LUT30_LSB                                            16
#define YCURVE_7_YCURVE_LUT31_MASK                                   0xff000000
#define YCURVE_7_YCURVE_LUT31_LSB                                            24

/* ISP_YCURVE_8 */
#define YCURVE_8_YCURVE_LUT32_MASK                                         0xff
#define YCURVE_8_YCURVE_LUT32_LSB                                             0
#define YCURVE_8_YCURVE_LUT33_MASK                                       0xff00
#define YCURVE_8_YCURVE_LUT33_LSB                                             8
#define YCURVE_8_YCURVE_LUT34_MASK                                     0xff0000
#define YCURVE_8_YCURVE_LUT34_LSB                                            16
#define YCURVE_8_YCURVE_LUT35_MASK                                   0xff000000
#define YCURVE_8_YCURVE_LUT35_LSB                                            24

/* ISP_YCURVE_9 */
#define YCURVE_9_YCURVE_LUT36_MASK                                         0xff
#define YCURVE_9_YCURVE_LUT36_LSB                                             0
#define YCURVE_9_YCURVE_LUT37_MASK                                       0xff00
#define YCURVE_9_YCURVE_LUT37_LSB                                             8
#define YCURVE_9_YCURVE_LUT38_MASK                                     0xff0000
#define YCURVE_9_YCURVE_LUT38_LSB                                            16
#define YCURVE_9_YCURVE_LUT39_MASK                                   0xff000000
#define YCURVE_9_YCURVE_LUT39_LSB                                            24

/* ISP_YCURVE_10 */
#define YCURVE_10_YCURVE_LUT40_MASK                                        0xff
#define YCURVE_10_YCURVE_LUT40_LSB                                            0
#define YCURVE_10_YCURVE_LUT41_MASK                                      0xff00
#define YCURVE_10_YCURVE_LUT41_LSB                                            8
#define YCURVE_10_YCURVE_LUT42_MASK                                    0xff0000
#define YCURVE_10_YCURVE_LUT42_LSB                                           16
#define YCURVE_10_YCURVE_LUT43_MASK                                  0xff000000
#define YCURVE_10_YCURVE_LUT43_LSB                                           24

/* ISP_YCURVE_11 */
#define YCURVE_11_YCURVE_LUT44_MASK                                        0xff
#define YCURVE_11_YCURVE_LUT44_LSB                                            0
#define YCURVE_11_YCURVE_LUT45_MASK                                      0xff00
#define YCURVE_11_YCURVE_LUT45_LSB                                            8
#define YCURVE_11_YCURVE_LUT46_MASK                                    0xff0000
#define YCURVE_11_YCURVE_LUT46_LSB                                           16
#define YCURVE_11_YCURVE_LUT47_MASK                                  0xff000000
#define YCURVE_11_YCURVE_LUT47_LSB                                           24

/* ISP_YCURVE_12 */
#define YCURVE_12_YCURVE_LUT48_MASK                                        0xff
#define YCURVE_12_YCURVE_LUT48_LSB                                            0
#define YCURVE_12_YCURVE_LUT49_MASK                                      0xff00
#define YCURVE_12_YCURVE_LUT49_LSB                                            8
#define YCURVE_12_YCURVE_LUT50_MASK                                    0xff0000
#define YCURVE_12_YCURVE_LUT50_LSB                                           16
#define YCURVE_12_YCURVE_LUT51_MASK                                  0xff000000
#define YCURVE_12_YCURVE_LUT51_LSB                                           24

/* ISP_YCURVE_13 */
#define YCURVE_13_YCURVE_LUT52_MASK                                        0xff
#define YCURVE_13_YCURVE_LUT52_LSB                                            0
#define YCURVE_13_YCURVE_LUT53_MASK                                      0xff00
#define YCURVE_13_YCURVE_LUT53_LSB                                            8
#define YCURVE_13_YCURVE_LUT54_MASK                                    0xff0000
#define YCURVE_13_YCURVE_LUT54_LSB                                           16
#define YCURVE_13_YCURVE_LUT55_MASK                                  0xff000000
#define YCURVE_13_YCURVE_LUT55_LSB                                           24

/* ISP_YCURVE_14 */
#define YCURVE_14_YCURVE_LUT56_MASK                                        0xff
#define YCURVE_14_YCURVE_LUT56_LSB                                            0
#define YCURVE_14_YCURVE_LUT57_MASK                                      0xff00
#define YCURVE_14_YCURVE_LUT57_LSB                                            8
#define YCURVE_14_YCURVE_LUT58_MASK                                    0xff0000
#define YCURVE_14_YCURVE_LUT58_LSB                                           16
#define YCURVE_14_YCURVE_LUT59_MASK                                  0xff000000
#define YCURVE_14_YCURVE_LUT59_LSB                                           24

/* ISP_YCURVE_15 */
#define YCURVE_15_YCURVE_LUT60_MASK                                        0xff
#define YCURVE_15_YCURVE_LUT60_LSB                                            0
#define YCURVE_15_YCURVE_LUT61_MASK                                      0xff00
#define YCURVE_15_YCURVE_LUT61_LSB                                            8
#define YCURVE_15_YCURVE_LUT62_MASK                                    0xff0000
#define YCURVE_15_YCURVE_LUT62_LSB                                           16
#define YCURVE_15_YCURVE_LUT63_MASK                                  0xff000000
#define YCURVE_15_YCURVE_LUT63_LSB                                           24

/* ISP_YCURVE_16 */
#define YCURVE_16_YCURVE_LUT64_MASK                                        0xff
#define YCURVE_16_YCURVE_LUT64_LSB                                            0
#define YCURVE_16_YCURVE_LUT65_MASK                                      0xff00
#define YCURVE_16_YCURVE_LUT65_LSB                                            8
#define YCURVE_16_YCURVE_LUT66_MASK                                    0xff0000
#define YCURVE_16_YCURVE_LUT66_LSB                                           16
#define YCURVE_16_YCURVE_LUT67_MASK                                  0xff000000
#define YCURVE_16_YCURVE_LUT67_LSB                                           24

/* ISP_YCURVE_17 */
#define YCURVE_17_YCURVE_LUT68_MASK                                        0xff
#define YCURVE_17_YCURVE_LUT68_LSB                                            0
#define YCURVE_17_YCURVE_LUT69_MASK                                      0xff00
#define YCURVE_17_YCURVE_LUT69_LSB                                            8
#define YCURVE_17_YCURVE_LUT70_MASK                                    0xff0000
#define YCURVE_17_YCURVE_LUT70_LSB                                           16
#define YCURVE_17_YCURVE_LUT71_MASK                                  0xff000000
#define YCURVE_17_YCURVE_LUT71_LSB                                           24

/* ISP_YCURVE_18 */
#define YCURVE_18_YCURVE_LUT72_MASK                                        0xff
#define YCURVE_18_YCURVE_LUT72_LSB                                            0
#define YCURVE_18_YCURVE_LUT73_MASK                                      0xff00
#define YCURVE_18_YCURVE_LUT73_LSB                                            8
#define YCURVE_18_YCURVE_LUT74_MASK                                    0xff0000
#define YCURVE_18_YCURVE_LUT74_LSB                                           16
#define YCURVE_18_YCURVE_LUT75_MASK                                  0xff000000
#define YCURVE_18_YCURVE_LUT75_LSB                                           24

/* ISP_YCURVE_19 */
#define YCURVE_19_YCURVE_LUT76_MASK                                        0xff
#define YCURVE_19_YCURVE_LUT76_LSB                                            0
#define YCURVE_19_YCURVE_LUT77_MASK                                      0xff00
#define YCURVE_19_YCURVE_LUT77_LSB                                            8
#define YCURVE_19_YCURVE_LUT78_MASK                                    0xff0000
#define YCURVE_19_YCURVE_LUT78_LSB                                           16
#define YCURVE_19_YCURVE_LUT79_MASK                                  0xff000000
#define YCURVE_19_YCURVE_LUT79_LSB                                           24

/* ISP_YCURVE_20 */
#define YCURVE_20_YCURVE_LUT80_MASK                                        0xff
#define YCURVE_20_YCURVE_LUT80_LSB                                            0
#define YCURVE_20_YCURVE_LUT81_MASK                                      0xff00
#define YCURVE_20_YCURVE_LUT81_LSB                                            8
#define YCURVE_20_YCURVE_LUT82_MASK                                    0xff0000
#define YCURVE_20_YCURVE_LUT82_LSB                                           16
#define YCURVE_20_YCURVE_LUT83_MASK                                  0xff000000
#define YCURVE_20_YCURVE_LUT83_LSB                                           24

/* ISP_YCURVE_21 */
#define YCURVE_21_YCURVE_LUT84_MASK                                        0xff
#define YCURVE_21_YCURVE_LUT84_LSB                                            0
#define YCURVE_21_YCURVE_LUT85_MASK                                      0xff00
#define YCURVE_21_YCURVE_LUT85_LSB                                            8
#define YCURVE_21_YCURVE_LUT86_MASK                                    0xff0000
#define YCURVE_21_YCURVE_LUT86_LSB                                           16
#define YCURVE_21_YCURVE_LUT87_MASK                                  0xff000000
#define YCURVE_21_YCURVE_LUT87_LSB                                           24

/* ISP_YCURVE_22 */
#define YCURVE_22_YCURVE_LUT88_MASK                                        0xff
#define YCURVE_22_YCURVE_LUT88_LSB                                            0
#define YCURVE_22_YCURVE_LUT89_MASK                                      0xff00
#define YCURVE_22_YCURVE_LUT89_LSB                                            8
#define YCURVE_22_YCURVE_LUT90_MASK                                    0xff0000
#define YCURVE_22_YCURVE_LUT90_LSB                                           16
#define YCURVE_22_YCURVE_LUT91_MASK                                  0xff000000
#define YCURVE_22_YCURVE_LUT91_LSB                                           24

/* ISP_YCURVE_23 */
#define YCURVE_23_YCURVE_LUT92_MASK                                        0xff
#define YCURVE_23_YCURVE_LUT92_LSB                                            0
#define YCURVE_23_YCURVE_LUT93_MASK                                      0xff00
#define YCURVE_23_YCURVE_LUT93_LSB                                            8
#define YCURVE_23_YCURVE_LUT94_MASK                                    0xff0000
#define YCURVE_23_YCURVE_LUT94_LSB                                           16
#define YCURVE_23_YCURVE_LUT95_MASK                                  0xff000000
#define YCURVE_23_YCURVE_LUT95_LSB                                           24

/* ISP_YCURVE_24 */
#define YCURVE_24_YCURVE_LUT96_MASK                                        0xff
#define YCURVE_24_YCURVE_LUT96_LSB                                            0
#define YCURVE_24_YCURVE_LUT97_MASK                                      0xff00
#define YCURVE_24_YCURVE_LUT97_LSB                                            8
#define YCURVE_24_YCURVE_LUT98_MASK                                    0xff0000
#define YCURVE_24_YCURVE_LUT98_LSB                                           16
#define YCURVE_24_YCURVE_LUT99_MASK                                  0xff000000
#define YCURVE_24_YCURVE_LUT99_LSB                                           24

/* ISP_YCURVE_25 */
#define YCURVE_25_YCURVE_LUT100_MASK                                       0xff
#define YCURVE_25_YCURVE_LUT100_LSB                                           0
#define YCURVE_25_YCURVE_LUT101_MASK                                     0xff00
#define YCURVE_25_YCURVE_LUT101_LSB                                           8
#define YCURVE_25_YCURVE_LUT102_MASK                                   0xff0000
#define YCURVE_25_YCURVE_LUT102_LSB                                          16
#define YCURVE_25_YCURVE_LUT103_MASK                                 0xff000000
#define YCURVE_25_YCURVE_LUT103_LSB                                          24

/* ISP_YCURVE_26 */
#define YCURVE_26_YCURVE_LUT104_MASK                                       0xff
#define YCURVE_26_YCURVE_LUT104_LSB                                           0
#define YCURVE_26_YCURVE_LUT105_MASK                                     0xff00
#define YCURVE_26_YCURVE_LUT105_LSB                                           8
#define YCURVE_26_YCURVE_LUT106_MASK                                   0xff0000
#define YCURVE_26_YCURVE_LUT106_LSB                                          16
#define YCURVE_26_YCURVE_LUT107_MASK                                 0xff000000
#define YCURVE_26_YCURVE_LUT107_LSB                                          24

/* ISP_YCURVE_27 */
#define YCURVE_27_YCURVE_LUT108_MASK                                       0xff
#define YCURVE_27_YCURVE_LUT108_LSB                                           0
#define YCURVE_27_YCURVE_LUT109_MASK                                     0xff00
#define YCURVE_27_YCURVE_LUT109_LSB                                           8
#define YCURVE_27_YCURVE_LUT110_MASK                                   0xff0000
#define YCURVE_27_YCURVE_LUT110_LSB                                          16
#define YCURVE_27_YCURVE_LUT111_MASK                                 0xff000000
#define YCURVE_27_YCURVE_LUT111_LSB                                          24

/* ISP_YCURVE_28 */
#define YCURVE_28_YCURVE_LUT112_MASK                                       0xff
#define YCURVE_28_YCURVE_LUT112_LSB                                           0
#define YCURVE_28_YCURVE_LUT113_MASK                                     0xff00
#define YCURVE_28_YCURVE_LUT113_LSB                                           8
#define YCURVE_28_YCURVE_LUT114_MASK                                   0xff0000
#define YCURVE_28_YCURVE_LUT114_LSB                                          16
#define YCURVE_28_YCURVE_LUT115_MASK                                 0xff000000
#define YCURVE_28_YCURVE_LUT115_LSB                                          24

/* ISP_YCURVE_29 */
#define YCURVE_29_YCURVE_LUT116_MASK                                       0xff
#define YCURVE_29_YCURVE_LUT116_LSB                                           0
#define YCURVE_29_YCURVE_LUT117_MASK                                     0xff00
#define YCURVE_29_YCURVE_LUT117_LSB                                           8
#define YCURVE_29_YCURVE_LUT118_MASK                                   0xff0000
#define YCURVE_29_YCURVE_LUT118_LSB                                          16
#define YCURVE_29_YCURVE_LUT119_MASK                                 0xff000000
#define YCURVE_29_YCURVE_LUT119_LSB                                          24

/* ISP_YCURVE_30 */
#define YCURVE_30_YCURVE_LUT120_MASK                                       0xff
#define YCURVE_30_YCURVE_LUT120_LSB                                           0
#define YCURVE_30_YCURVE_LUT121_MASK                                     0xff00
#define YCURVE_30_YCURVE_LUT121_LSB                                           8
#define YCURVE_30_YCURVE_LUT122_MASK                                   0xff0000
#define YCURVE_30_YCURVE_LUT122_LSB                                          16
#define YCURVE_30_YCURVE_LUT123_MASK                                 0xff000000
#define YCURVE_30_YCURVE_LUT123_LSB                                          24

/* ISP_YCURVE_31 */
#define YCURVE_31_YCURVE_LUT124_MASK                                       0xff
#define YCURVE_31_YCURVE_LUT124_LSB                                           0
#define YCURVE_31_YCURVE_LUT125_MASK                                     0xff00
#define YCURVE_31_YCURVE_LUT125_LSB                                           8
#define YCURVE_31_YCURVE_LUT126_MASK                                   0xff0000
#define YCURVE_31_YCURVE_LUT126_LSB                                          16
#define YCURVE_31_YCURVE_LUT127_MASK                                 0xff000000
#define YCURVE_31_YCURVE_LUT127_LSB                                          24

/* ISP_YCURVE_32 */
#define YCURVE_32_YCURVE_LUT128_MASK                                       0xff
#define YCURVE_32_YCURVE_LUT128_LSB                                           0

/* ISP_SP_0 */
#define SP_0_SP_STR_GLOBAL_POS_MASK                                       0x1ff
#define SP_0_SP_STR_GLOBAL_POS_LSB                                            0
#define SP_0_SP_STR_GLOBAL_NEG_MASK                                   0x1ff0000
#define SP_0_SP_STR_GLOBAL_NEG_LSB                                           16

/* ISP_SP_1 */
#define SP_1_SP_STR_DIREDGE_MASK                                           0xff
#define SP_1_SP_STR_DIREDGE_LSB                                               0
#define SP_1_SP_STR_TEXTURE_MASK                                         0xff00
#define SP_1_SP_STR_TEXTURE_LSB                                               8
#define SP_1_SP_STR_FLAT_MASK                                          0xff0000
#define SP_1_SP_STR_FLAT_LSB                                                 16

/* ISP_SP_2 */
#define SP_2_SP_STR_OVERSHOOT_MASK                                         0xff
#define SP_2_SP_STR_OVERSHOOT_LSB                                             0
#define SP_2_SP_STR_UNDERSHOOT_MASK                                    0xff0000
#define SP_2_SP_STR_UNDERSHOOT_LSB                                           16

/* ISP_SP_3 */
#define SP_3_SP_SAD_LPF_WT_MASK                                            0x3f
#define SP_3_SP_SAD_LPF_WT_LSB                                                0

/* ISP_SP_4 */
#define SP_4_SP_SAD_CLAMP_VAL_MASK                                       0x1fff
#define SP_4_SP_SAD_CLAMP_VAL_LSB                                             0
#define SP_4_SP_SAD_CLAMP_INV_NORM_MASK                              0xffff0000
#define SP_4_SP_SAD_CLAMP_INV_NORM_LSB                                       16

/* ISP_SP_5 */
#define SP_5_SP_CORING_TH_MAIN_SUB_MASK                                    0xff
#define SP_5_SP_CORING_TH_MAIN_SUB_LSB                                        0
#define SP_5_SP_GAIN_MAIN_SUB_MASK                                       0xff00
#define SP_5_SP_GAIN_MAIN_SUB_LSB                                             8
#define SP_5_SP_WT_LOW_MAIN_SUB_MASK                                   0x3f0000
#define SP_5_SP_WT_LOW_MAIN_SUB_LSB                                          16
#define SP_5_SP_WT_HIGH_MAIN_SUB_MASK                                0x3f000000
#define SP_5_SP_WT_HIGH_MAIN_SUB_LSB                                         24

/* ISP_SP_6 */
#define SP_6_SP_CORING_TH_DIREDGE_MASK                                     0xff
#define SP_6_SP_CORING_TH_DIREDGE_LSB                                         0
#define SP_6_SP_GAIN_DIREDGE_MASK                                        0xff00
#define SP_6_SP_GAIN_DIREDGE_LSB                                              8
#define SP_6_SP_WT_LOW_DIREDGE_MASK                                    0x3f0000
#define SP_6_SP_WT_LOW_DIREDGE_LSB                                           16
#define SP_6_SP_WT_HIGH_DIREDGE_MASK                                 0x3f000000
#define SP_6_SP_WT_HIGH_DIREDGE_LSB                                          24

/* ISP_SP_7 */
#define SP_7_SP_CORING_TH_2D_FLAT_MASK                                     0xff
#define SP_7_SP_CORING_TH_2D_FLAT_LSB                                         0
#define SP_7_SP_GAIN_2D_FLAT_MASK                                        0xff00
#define SP_7_SP_GAIN_2D_FLAT_LSB                                              8
#define SP_7_SP_WT_LOW_2D_FLAT_MASK                                    0x3f0000
#define SP_7_SP_WT_LOW_2D_FLAT_LSB                                           16
#define SP_7_SP_WT_HIGH_2D_FLAT_MASK                                 0x3f000000
#define SP_7_SP_WT_HIGH_2D_FLAT_LSB                                          24

/* ISP_SP_8 */
#define SP_8_SP_FILT_HV_W1_MASK                                            0xff
#define SP_8_SP_FILT_HV_W1_LSB                                                0
#define SP_8_SP_FILT_HV_W2_MASK                                          0xff00
#define SP_8_SP_FILT_HV_W2_LSB                                                8
#define SP_8_SP_FILT_HV_W3_MASK                                        0xff0000
#define SP_8_SP_FILT_HV_W3_LSB                                               16
#define SP_8_SP_FILT_HV_W4_MASK                                      0xff000000
#define SP_8_SP_FILT_HV_W4_LSB                                               24

/* ISP_SP_9 */
#define SP_9_SP_FILT_HV_W5_MASK                                            0xff
#define SP_9_SP_FILT_HV_W5_LSB                                                0
#define SP_9_SP_FILT_HV_W6_MASK                                          0xff00
#define SP_9_SP_FILT_HV_W6_LSB                                                8
#define SP_9_SP_FILT_HV_W7_MASK                                        0xff0000
#define SP_9_SP_FILT_HV_W7_LSB                                               16
#define SP_9_SP_FILT_HV_W8_MASK                                      0xff000000
#define SP_9_SP_FILT_HV_W8_LSB                                               24

/* ISP_SP_10 */
#define SP_10_SP_FILT_HV_W9_MASK                                           0xff
#define SP_10_SP_FILT_HV_W9_LSB                                               0

/* ISP_SP_11 */
#define SP_11_SP_FILT_DG_W1_MASK                                           0xff
#define SP_11_SP_FILT_DG_W1_LSB                                               0
#define SP_11_SP_FILT_DG_W2_MASK                                         0xff00
#define SP_11_SP_FILT_DG_W2_LSB                                               8
#define SP_11_SP_FILT_DG_W3_MASK                                       0xff0000
#define SP_11_SP_FILT_DG_W3_LSB                                              16
#define SP_11_SP_FILT_DG_W4_MASK                                     0xff000000
#define SP_11_SP_FILT_DG_W4_LSB                                              24

/* ISP_SP_12 */
#define SP_12_SP_FILT_DG_W5_MASK                                           0xff
#define SP_12_SP_FILT_DG_W5_LSB                                               0
#define SP_12_SP_FILT_DG_W6_MASK                                         0xff00
#define SP_12_SP_FILT_DG_W6_LSB                                               8
#define SP_12_SP_FILT_DG_W7_MASK                                       0xff0000
#define SP_12_SP_FILT_DG_W7_LSB                                              16
#define SP_12_SP_FILT_DG_W8_MASK                                     0xff000000
#define SP_12_SP_FILT_DG_W8_LSB                                              24

/* ISP_SP_13 */
#define SP_13_SP_FILT_DG_W9_MASK                                           0xff
#define SP_13_SP_FILT_DG_W9_LSB                                               0

/* ISP_SP_14 */
#define SP_14_SP_FILT_TEXTURE_W1_MASK                                      0xff
#define SP_14_SP_FILT_TEXTURE_W1_LSB                                          0
#define SP_14_SP_FILT_TEXTURE_W2_MASK                                    0xff00
#define SP_14_SP_FILT_TEXTURE_W2_LSB                                          8
#define SP_14_SP_FILT_TEXTURE_W3_MASK                                  0xff0000
#define SP_14_SP_FILT_TEXTURE_W3_LSB                                         16
#define SP_14_SP_FILT_TEXTURE_W4_MASK                                0xff000000
#define SP_14_SP_FILT_TEXTURE_W4_LSB                                         24

/* ISP_SP_15 */
#define SP_15_SP_FILT_TEXTURE_W5_MASK                                      0xff
#define SP_15_SP_FILT_TEXTURE_W5_LSB                                          0
#define SP_15_SP_FILT_TEXTURE_W6_MASK                                    0xff00
#define SP_15_SP_FILT_TEXTURE_W6_LSB                                          8

/* ISP_SP_16 */
#define SP_16_SP_FILT_FLAT_W1_MASK                                         0xff
#define SP_16_SP_FILT_FLAT_W1_LSB                                             0
#define SP_16_SP_FILT_FLAT_W2_MASK                                       0xff00
#define SP_16_SP_FILT_FLAT_W2_LSB                                             8
#define SP_16_SP_FILT_FLAT_W3_MASK                                     0xff0000
#define SP_16_SP_FILT_FLAT_W3_LSB                                            16
#define SP_16_SP_FILT_FLAT_W4_MASK                                   0xff000000
#define SP_16_SP_FILT_FLAT_W4_LSB                                            24

/* ISP_SP_17 */
#define SP_17_SP_FILT_FLAT_W5_MASK                                         0xff
#define SP_17_SP_FILT_FLAT_W5_LSB                                             0
#define SP_17_SP_FILT_FLAT_W6_MASK                                       0xff00
#define SP_17_SP_FILT_FLAT_W6_LSB                                             8

/* ISP_SP_18 */
#define SP_18_SP_FILT_HV_CORING_TH_MASK                                   0x3ff
#define SP_18_SP_FILT_HV_CORING_TH_LSB                                        0
#define SP_18_SP_FILT_HV_ADJGAIN_MASK                                 0x1ff0000
#define SP_18_SP_FILT_HV_ADJGAIN_LSB                                         16
#define SP_18_SP_FILT_HV_ADJSHIFT_MASK                               0xf0000000
#define SP_18_SP_FILT_HV_ADJSHIFT_LSB                                        28

/* ISP_SP_19 */
#define SP_19_SP_FILT_DG_CORING_TH_MASK                                   0x3ff
#define SP_19_SP_FILT_DG_CORING_TH_LSB                                        0
#define SP_19_SP_FILT_DG_ADJGAIN_MASK                                 0x1ff0000
#define SP_19_SP_FILT_DG_ADJGAIN_LSB                                         16
#define SP_19_SP_FILT_DG_ADJSHIFT_MASK                               0xf0000000
#define SP_19_SP_FILT_DG_ADJSHIFT_LSB                                        28

/* ISP_SP_20 */
#define SP_20_SP_FILT_TEXTURE_CORING_TH_MASK                              0x3ff
#define SP_20_SP_FILT_TEXTURE_CORING_TH_LSB                                   0
#define SP_20_SP_FILT_TEXTURE_ADJGAIN_MASK                            0x1ff0000
#define SP_20_SP_FILT_TEXTURE_ADJGAIN_LSB                                    16
#define SP_20_SP_FILT_TEXTURE_ADJSHIFT_MASK                          0xf0000000
#define SP_20_SP_FILT_TEXTURE_ADJSHIFT_LSB                                   28

/* ISP_SP_21 */
#define SP_21_SP_FILT_FLAT_CORING_TH_MASK                                 0x3ff
#define SP_21_SP_FILT_FLAT_CORING_TH_LSB                                      0
#define SP_21_SP_FILT_FLAT_ADJGAIN_MASK                               0x1ff0000
#define SP_21_SP_FILT_FLAT_ADJGAIN_LSB                                       16
#define SP_21_SP_FILT_FLAT_ADJSHIFT_MASK                             0xf0000000
#define SP_21_SP_FILT_FLAT_ADJSHIFT_LSB                                      28

/* ISP_SP_22 */
#define SP_22_SP_LOCALSTR_DIREDGE0_MASK                                    0xff
#define SP_22_SP_LOCALSTR_DIREDGE0_LSB                                        0
#define SP_22_SP_LOCALSTR_DIREDGE1_MASK                                  0xff00
#define SP_22_SP_LOCALSTR_DIREDGE1_LSB                                        8
#define SP_22_SP_LOCALSTR_DIREDGE2_MASK                                0xff0000
#define SP_22_SP_LOCALSTR_DIREDGE2_LSB                                       16
#define SP_22_SP_LOCALSTR_DIREDGE3_MASK                              0xff000000
#define SP_22_SP_LOCALSTR_DIREDGE3_LSB                                       24

/* ISP_SP_23 */
#define SP_23_SP_LOCALSTR_DIREDGE4_MASK                                    0xff
#define SP_23_SP_LOCALSTR_DIREDGE4_LSB                                        0
#define SP_23_SP_LOCALSTR_DIREDGE5_MASK                                  0xff00
#define SP_23_SP_LOCALSTR_DIREDGE5_LSB                                        8
#define SP_23_SP_LOCALSTR_DIREDGE6_MASK                                0xff0000
#define SP_23_SP_LOCALSTR_DIREDGE6_LSB                                       16
#define SP_23_SP_LOCALSTR_DIREDGE7_MASK                              0xff000000
#define SP_23_SP_LOCALSTR_DIREDGE7_LSB                                       24

/* ISP_SP_24 */
#define SP_24_SP_LOCALSTR_DIREDGE8_MASK                                    0xff
#define SP_24_SP_LOCALSTR_DIREDGE8_LSB                                        0
#define SP_24_SP_LOCALSTR_DIREDGE9_MASK                                  0xff00
#define SP_24_SP_LOCALSTR_DIREDGE9_LSB                                        8
#define SP_24_SP_LOCALSTR_DIREDGE10_MASK                               0xff0000
#define SP_24_SP_LOCALSTR_DIREDGE10_LSB                                      16
#define SP_24_SP_LOCALSTR_DIREDGE11_MASK                             0xff000000
#define SP_24_SP_LOCALSTR_DIREDGE11_LSB                                      24

/* ISP_SP_25 */
#define SP_25_SP_LOCALSTR_DIREDGE12_MASK                                   0xff
#define SP_25_SP_LOCALSTR_DIREDGE12_LSB                                       0
#define SP_25_SP_LOCALSTR_DIREDGE13_MASK                                 0xff00
#define SP_25_SP_LOCALSTR_DIREDGE13_LSB                                       8
#define SP_25_SP_LOCALSTR_DIREDGE14_MASK                               0xff0000
#define SP_25_SP_LOCALSTR_DIREDGE14_LSB                                      16
#define SP_25_SP_LOCALSTR_DIREDGE15_MASK                             0xff000000
#define SP_25_SP_LOCALSTR_DIREDGE15_LSB                                      24

/* ISP_SP_26 */
#define SP_26_SP_LOCALSTR_DIREDGE16_MASK                                   0xff
#define SP_26_SP_LOCALSTR_DIREDGE16_LSB                                       0

/* ISP_SP_27 */
#define SP_27_SP_LOCALSTR_NONDIREDGE0_MASK                                 0xff
#define SP_27_SP_LOCALSTR_NONDIREDGE0_LSB                                     0
#define SP_27_SP_LOCALSTR_NONDIREDGE1_MASK                               0xff00
#define SP_27_SP_LOCALSTR_NONDIREDGE1_LSB                                     8
#define SP_27_SP_LOCALSTR_NONDIREDGE2_MASK                             0xff0000
#define SP_27_SP_LOCALSTR_NONDIREDGE2_LSB                                    16
#define SP_27_SP_LOCALSTR_NONDIREDGE3_MASK                           0xff000000
#define SP_27_SP_LOCALSTR_NONDIREDGE3_LSB                                    24

/* ISP_SP_28 */
#define SP_28_SP_LOCALSTR_NONDIREDGE4_MASK                                 0xff
#define SP_28_SP_LOCALSTR_NONDIREDGE4_LSB                                     0
#define SP_28_SP_LOCALSTR_NONDIREDGE5_MASK                               0xff00
#define SP_28_SP_LOCALSTR_NONDIREDGE5_LSB                                     8
#define SP_28_SP_LOCALSTR_NONDIREDGE6_MASK                             0xff0000
#define SP_28_SP_LOCALSTR_NONDIREDGE6_LSB                                    16
#define SP_28_SP_LOCALSTR_NONDIREDGE7_MASK                           0xff000000
#define SP_28_SP_LOCALSTR_NONDIREDGE7_LSB                                    24

/* ISP_SP_29 */
#define SP_29_SP_LOCALSTR_NONDIREDGE8_MASK                                 0xff
#define SP_29_SP_LOCALSTR_NONDIREDGE8_LSB                                     0
#define SP_29_SP_LOCALSTR_NONDIREDGE9_MASK                               0xff00
#define SP_29_SP_LOCALSTR_NONDIREDGE9_LSB                                     8
#define SP_29_SP_LOCALSTR_NONDIREDGE10_MASK                            0xff0000
#define SP_29_SP_LOCALSTR_NONDIREDGE10_LSB                                   16
#define SP_29_SP_LOCALSTR_NONDIREDGE11_MASK                          0xff000000
#define SP_29_SP_LOCALSTR_NONDIREDGE11_LSB                                   24

/* ISP_SP_30 */
#define SP_30_SP_LOCALSTR_NONDIREDGE12_MASK                                0xff
#define SP_30_SP_LOCALSTR_NONDIREDGE12_LSB                                    0
#define SP_30_SP_LOCALSTR_NONDIREDGE13_MASK                              0xff00
#define SP_30_SP_LOCALSTR_NONDIREDGE13_LSB                                    8
#define SP_30_SP_LOCALSTR_NONDIREDGE14_MASK                            0xff0000
#define SP_30_SP_LOCALSTR_NONDIREDGE14_LSB                                   16
#define SP_30_SP_LOCALSTR_NONDIREDGE15_MASK                          0xff000000
#define SP_30_SP_LOCALSTR_NONDIREDGE15_LSB                                   24

/* ISP_SP_31 */
#define SP_31_SP_LOCALSTR_NONDIREDGE16_MASK                                0xff
#define SP_31_SP_LOCALSTR_NONDIREDGE16_LSB                                    0

/* ISP_SP_32 */
#define SP_32_SP_LOCALSTR_LUMA0_MASK                                       0xff
#define SP_32_SP_LOCALSTR_LUMA0_LSB                                           0
#define SP_32_SP_LOCALSTR_LUMA1_MASK                                     0xff00
#define SP_32_SP_LOCALSTR_LUMA1_LSB                                           8
#define SP_32_SP_LOCALSTR_LUMA2_MASK                                   0xff0000
#define SP_32_SP_LOCALSTR_LUMA2_LSB                                          16
#define SP_32_SP_LOCALSTR_LUMA3_MASK                                 0xff000000
#define SP_32_SP_LOCALSTR_LUMA3_LSB                                          24

/* ISP_SP_33 */
#define SP_33_SP_LOCALSTR_LUMA4_MASK                                       0xff
#define SP_33_SP_LOCALSTR_LUMA4_LSB                                           0
#define SP_33_SP_LOCALSTR_LUMA5_MASK                                     0xff00
#define SP_33_SP_LOCALSTR_LUMA5_LSB                                           8
#define SP_33_SP_LOCALSTR_LUMA6_MASK                                   0xff0000
#define SP_33_SP_LOCALSTR_LUMA6_LSB                                          16
#define SP_33_SP_LOCALSTR_LUMA7_MASK                                 0xff000000
#define SP_33_SP_LOCALSTR_LUMA7_LSB                                          24

/* ISP_SP_34 */
#define SP_34_SP_LOCALSTR_LUMA8_MASK                                       0xff
#define SP_34_SP_LOCALSTR_LUMA8_LSB                                           0
#define SP_34_SP_LOCALSTR_LUMA9_MASK                                     0xff00
#define SP_34_SP_LOCALSTR_LUMA9_LSB                                           8
#define SP_34_SP_LOCALSTR_LUMA10_MASK                                  0xff0000
#define SP_34_SP_LOCALSTR_LUMA10_LSB                                         16
#define SP_34_SP_LOCALSTR_LUMA11_MASK                                0xff000000
#define SP_34_SP_LOCALSTR_LUMA11_LSB                                         24

/* ISP_SP_35 */
#define SP_35_SP_LOCALSTR_LUMA12_MASK                                      0xff
#define SP_35_SP_LOCALSTR_LUMA12_LSB                                          0
#define SP_35_SP_LOCALSTR_LUMA13_MASK                                    0xff00
#define SP_35_SP_LOCALSTR_LUMA13_LSB                                          8
#define SP_35_SP_LOCALSTR_LUMA14_MASK                                  0xff0000
#define SP_35_SP_LOCALSTR_LUMA14_LSB                                         16
#define SP_35_SP_LOCALSTR_LUMA15_MASK                                0xff000000
#define SP_35_SP_LOCALSTR_LUMA15_LSB                                         24

/* ISP_SP_36 */
#define SP_36_SP_LOCALSTR_LUMA16_MASK                                      0xff
#define SP_36_SP_LOCALSTR_LUMA16_LSB                                          0

/* ISP_NR3D_0 */
#define NR3D_0_NR3D_SAD_RATIO_MASK                                         0x7f
#define NR3D_0_NR3D_SAD_RATIO_LSB                                             0

/* ISP_NR3D_1 */
#define NR3D_1_NR3D_SAD_STD_LUT0_MASK                                    0x3fff
#define NR3D_1_NR3D_SAD_STD_LUT0_LSB                                          0
#define NR3D_1_NR3D_SAD_STD_LUT1_MASK                                0x3fff0000
#define NR3D_1_NR3D_SAD_STD_LUT1_LSB                                         16

/* ISP_NR3D_2 */
#define NR3D_2_NR3D_SAD_STD_LUT2_MASK                                    0x3fff
#define NR3D_2_NR3D_SAD_STD_LUT2_LSB                                          0
#define NR3D_2_NR3D_SAD_STD_LUT3_MASK                                0x3fff0000
#define NR3D_2_NR3D_SAD_STD_LUT3_LSB                                         16

/* ISP_NR3D_3 */
#define NR3D_3_NR3D_SAD_STD_LUT4_MASK                                    0x3fff
#define NR3D_3_NR3D_SAD_STD_LUT4_LSB                                          0
#define NR3D_3_NR3D_SAD_STD_LUT5_MASK                                0x3fff0000
#define NR3D_3_NR3D_SAD_STD_LUT5_LSB                                         16

/* ISP_NR3D_4 */
#define NR3D_4_NR3D_SAD_STD_LUT6_MASK                                    0x3fff
#define NR3D_4_NR3D_SAD_STD_LUT6_LSB                                          0
#define NR3D_4_NR3D_SAD_STD_LUT7_MASK                                0x3fff0000
#define NR3D_4_NR3D_SAD_STD_LUT7_LSB                                         16

/* ISP_NR3D_5 */
#define NR3D_5_NR3D_SAD_STD_LUT8_MASK                                    0x3fff
#define NR3D_5_NR3D_SAD_STD_LUT8_LSB                                          0

/* ISP_NR3D_6 */
#define NR3D_6_NR3D_COEFF_B_LUT0_MASK                                    0x3fff
#define NR3D_6_NR3D_COEFF_B_LUT0_LSB                                          0
#define NR3D_6_NR3D_COEFF_B_LUT1_MASK                                0x3fff0000
#define NR3D_6_NR3D_COEFF_B_LUT1_LSB                                         16

/* ISP_NR3D_7 */
#define NR3D_7_NR3D_COEFF_B_LUT2_MASK                                    0x3fff
#define NR3D_7_NR3D_COEFF_B_LUT2_LSB                                          0
#define NR3D_7_NR3D_COEFF_B_LUT3_MASK                                0x3fff0000
#define NR3D_7_NR3D_COEFF_B_LUT3_LSB                                         16

/* ISP_NR3D_8 */
#define NR3D_8_NR3D_COEFF_B_LUT4_MASK                                    0x3fff
#define NR3D_8_NR3D_COEFF_B_LUT4_LSB                                          0
#define NR3D_8_NR3D_COEFF_B_LUT5_MASK                                0x3fff0000
#define NR3D_8_NR3D_COEFF_B_LUT5_LSB                                         16

/* ISP_NR3D_9 */
#define NR3D_9_NR3D_COEFF_B_LUT6_MASK                                    0x3fff
#define NR3D_9_NR3D_COEFF_B_LUT6_LSB                                          0
#define NR3D_9_NR3D_COEFF_B_LUT7_MASK                                0x3fff0000
#define NR3D_9_NR3D_COEFF_B_LUT7_LSB                                         16

/* ISP_NR3D_10 */
#define NR3D_10_NR3D_COEFF_B_LUT8_MASK                                   0x3fff
#define NR3D_10_NR3D_COEFF_B_LUT8_LSB                                         0

/* ISP_NR3D_11 */
#define NR3D_11_NR3D_COEFF_A_LUT0_MASK                                     0x3f
#define NR3D_11_NR3D_COEFF_A_LUT0_LSB                                         0
#define NR3D_11_NR3D_COEFF_A_LUT1_MASK                                   0x3f00
#define NR3D_11_NR3D_COEFF_A_LUT1_LSB                                         8
#define NR3D_11_NR3D_COEFF_A_LUT2_MASK                                 0x3f0000
#define NR3D_11_NR3D_COEFF_A_LUT2_LSB                                        16
#define NR3D_11_NR3D_COEFF_A_LUT3_MASK                               0x3f000000
#define NR3D_11_NR3D_COEFF_A_LUT3_LSB                                        24

/* ISP_NR3D_12 */
#define NR3D_12_NR3D_COEFF_A_LUT4_MASK                                     0x3f
#define NR3D_12_NR3D_COEFF_A_LUT4_LSB                                         0
#define NR3D_12_NR3D_COEFF_A_LUT5_MASK                                   0x3f00
#define NR3D_12_NR3D_COEFF_A_LUT5_LSB                                         8
#define NR3D_12_NR3D_COEFF_A_LUT6_MASK                                 0x3f0000
#define NR3D_12_NR3D_COEFF_A_LUT6_LSB                                        16
#define NR3D_12_NR3D_COEFF_A_LUT7_MASK                               0x3f000000
#define NR3D_12_NR3D_COEFF_A_LUT7_LSB                                        24

/* ISP_NR3D_13 */
#define NR3D_13_NR3D_COEFF_A_LUT8_MASK                                     0x3f
#define NR3D_13_NR3D_COEFF_A_LUT8_LSB                                         0

/* ISP_NR3D_14 */
#define NR3D_14_NR3D_K1_MASK                                               0x3f
#define NR3D_14_NR3D_K1_LSB                                                   0
#define NR3D_14_NR3D_K2_MASK                                             0x3f00
#define NR3D_14_NR3D_K2_LSB                                                   8
#define NR3D_14_NR3D_UV_DIFF_MASK                                      0xff0000
#define NR3D_14_NR3D_UV_DIFF_LSB                                             16
#define NR3D_14_NR3D_UV_GAIN_MASK                                    0x3f000000
#define NR3D_14_NR3D_UV_GAIN_LSB                                             24

/* ISP_NR3D_15 */
#define NR3D_15_NR3D_DURATION_MOTION_MASK                                  0xff
#define NR3D_15_NR3D_DURATION_MOTION_LSB                                      0
#define NR3D_15_NR3D_DURATION_TRANSITION_MASK                            0xff00
#define NR3D_15_NR3D_DURATION_TRANSITION_LSB                                  8
#define NR3D_15_NR3D_DECAY_RATE_MASK                                   0xff0000
#define NR3D_15_NR3D_DECAY_RATE_LSB                                          16

/* ISP_NR3D_17 */
#define NR3D_17_NR3D_TF_Y_DELTA_LUT0_MASK                                  0xff
#define NR3D_17_NR3D_TF_Y_DELTA_LUT0_LSB                                      0
#define NR3D_17_NR3D_TF_Y_DELTA_LUT1_MASK                                0xff00
#define NR3D_17_NR3D_TF_Y_DELTA_LUT1_LSB                                      8
#define NR3D_17_NR3D_TF_Y_DELTA_LUT2_MASK                              0xff0000
#define NR3D_17_NR3D_TF_Y_DELTA_LUT2_LSB                                     16
#define NR3D_17_NR3D_TF_Y_DELTA_LUT3_MASK                            0xff000000
#define NR3D_17_NR3D_TF_Y_DELTA_LUT3_LSB                                     24

/* ISP_NR3D_18 */
#define NR3D_18_NR3D_TF_Y_DELTA_LUT4_MASK                                  0xff
#define NR3D_18_NR3D_TF_Y_DELTA_LUT4_LSB                                      0
#define NR3D_18_NR3D_TF_Y_DELTA_LUT5_MASK                                0xff00
#define NR3D_18_NR3D_TF_Y_DELTA_LUT5_LSB                                      8
#define NR3D_18_NR3D_TF_Y_DELTA_LUT6_MASK                              0xff0000
#define NR3D_18_NR3D_TF_Y_DELTA_LUT6_LSB                                     16
#define NR3D_18_NR3D_TF_Y_DELTA_LUT7_MASK                            0xff000000
#define NR3D_18_NR3D_TF_Y_DELTA_LUT7_LSB                                     24

/* ISP_NR3D_19 */
#define NR3D_19_NR3D_TF_Y_DELTA_LUT8_MASK                                  0xff
#define NR3D_19_NR3D_TF_Y_DELTA_LUT8_LSB                                      0

/* ISP_NR3D_20 */
#define NR3D_20_NR3D_TF_UV_DELTA_LUT0_MASK                                 0xff
#define NR3D_20_NR3D_TF_UV_DELTA_LUT0_LSB                                     0
#define NR3D_20_NR3D_TF_UV_DELTA_LUT1_MASK                               0xff00
#define NR3D_20_NR3D_TF_UV_DELTA_LUT1_LSB                                     8
#define NR3D_20_NR3D_TF_UV_DELTA_LUT2_MASK                             0xff0000
#define NR3D_20_NR3D_TF_UV_DELTA_LUT2_LSB                                    16
#define NR3D_20_NR3D_TF_UV_DELTA_LUT3_MASK                           0xff000000
#define NR3D_20_NR3D_TF_UV_DELTA_LUT3_LSB                                    24

/* ISP_NR3D_21 */
#define NR3D_21_NR3D_TF_UV_DELTA_LUT4_MASK                                 0xff
#define NR3D_21_NR3D_TF_UV_DELTA_LUT4_LSB                                     0
#define NR3D_21_NR3D_TF_UV_DELTA_LUT5_MASK                               0xff00
#define NR3D_21_NR3D_TF_UV_DELTA_LUT5_LSB                                     8
#define NR3D_21_NR3D_TF_UV_DELTA_LUT6_MASK                             0xff0000
#define NR3D_21_NR3D_TF_UV_DELTA_LUT6_LSB                                    16
#define NR3D_21_NR3D_TF_UV_DELTA_LUT7_MASK                           0xff000000
#define NR3D_21_NR3D_TF_UV_DELTA_LUT7_LSB                                    24

/* ISP_NR3D_22 */
#define NR3D_22_NR3D_TF_UV_DELTA_LUT8_MASK                                 0xff
#define NR3D_22_NR3D_TF_UV_DELTA_LUT8_LSB                                     0

/* ISP_NR3D_23 */
#define NR3D_23_NR3D_SF_FILT_W0_MASK                                       0x1f
#define NR3D_23_NR3D_SF_FILT_W0_LSB                                           0
#define NR3D_23_NR3D_SF_FILT_W1_MASK                                     0x1f00
#define NR3D_23_NR3D_SF_FILT_W1_LSB                                           8
#define NR3D_23_NR3D_SF_FILT_W2_MASK                                   0x1f0000
#define NR3D_23_NR3D_SF_FILT_W2_LSB                                          16
#define NR3D_23_NR3D_SF_FILT_W3_MASK                                 0x1f000000
#define NR3D_23_NR3D_SF_FILT_W3_LSB                                          24

/* ISP_NR3D_24 */
#define NR3D_24_NR3D_SF_FILT_W4_MASK                                       0x1f
#define NR3D_24_NR3D_SF_FILT_W4_LSB                                           0
#define NR3D_24_NR3D_SF_FILT_W5_MASK                                     0x1f00
#define NR3D_24_NR3D_SF_FILT_W5_LSB                                           8

/* ISP_NR3D_25 */
#define NR3D_25_NR3D_SF_RANGE_LUT0_MASK                                    0xff
#define NR3D_25_NR3D_SF_RANGE_LUT0_LSB                                        0
#define NR3D_25_NR3D_SF_RANGE_LUT1_MASK                                  0xff00
#define NR3D_25_NR3D_SF_RANGE_LUT1_LSB                                        8
#define NR3D_25_NR3D_SF_RANGE_LUT2_MASK                                0xff0000
#define NR3D_25_NR3D_SF_RANGE_LUT2_LSB                                       16
#define NR3D_25_NR3D_SF_RANGE_LUT3_MASK                              0xff000000
#define NR3D_25_NR3D_SF_RANGE_LUT3_LSB                                       24

/* ISP_NR3D_26 */
#define NR3D_26_NR3D_SF_RANGE_LUT4_MASK                                    0xff
#define NR3D_26_NR3D_SF_RANGE_LUT4_LSB                                        0
#define NR3D_26_NR3D_SF_RANGE_LUT5_MASK                                  0xff00
#define NR3D_26_NR3D_SF_RANGE_LUT5_LSB                                        8

/* ISP_NR3D_27 */
#define NR3D_27_NR3D_SF_LUM_LUT0_MASK                                      0xff
#define NR3D_27_NR3D_SF_LUM_LUT0_LSB                                          0
#define NR3D_27_NR3D_SF_LUM_LUT1_MASK                                    0xff00
#define NR3D_27_NR3D_SF_LUM_LUT1_LSB                                          8
#define NR3D_27_NR3D_SF_LUM_LUT2_MASK                                  0xff0000
#define NR3D_27_NR3D_SF_LUM_LUT2_LSB                                         16
#define NR3D_27_NR3D_SF_LUM_LUT3_MASK                                0xff000000
#define NR3D_27_NR3D_SF_LUM_LUT3_LSB                                         24

/* ISP_NR3D_28 */
#define NR3D_28_NR3D_SF_LUM_LUT4_MASK                                      0xff
#define NR3D_28_NR3D_SF_LUM_LUT4_LSB                                          0
#define NR3D_28_NR3D_SF_LUM_LUT5_MASK                                    0xff00
#define NR3D_28_NR3D_SF_LUM_LUT5_LSB                                          8
#define NR3D_28_NR3D_SF_LUM_LUT6_MASK                                  0xff0000
#define NR3D_28_NR3D_SF_LUM_LUT6_LSB                                         16
#define NR3D_28_NR3D_SF_LUM_LUT7_MASK                                0xff000000
#define NR3D_28_NR3D_SF_LUM_LUT7_LSB                                         24

/* ISP_NR3D_29 */
#define NR3D_29_NR3D_SF_LUM_LUT8_MASK                                      0xff
#define NR3D_29_NR3D_SF_LUM_LUT8_LSB                                          0

/* ISP_SCALER_0 */
#define SCALER_0_SRC_WD_MASK                                              0xfff
#define SCALER_0_SRC_WD_LSB                                                   0
#define SCALER_0_SRC_HT_MASK                                          0xfff0000
#define SCALER_0_SRC_HT_LSB                                                  16

/* ISP_SCALER_1 */
#define SCALER_1_DST_WDO_MASK                                             0xfff
#define SCALER_1_DST_WDO_LSB                                                  0
#define SCALER_1_DST_HTO_MASK                                         0xfff0000
#define SCALER_1_DST_HTO_LSB                                                 16

/* ISP_SCALER_2 */
#define SCALER_2_X_RATIO_INV_MASK                                       0x1ffff
#define SCALER_2_X_RATIO_INV_LSB                                              0

/* ISP_SCALER_3 */
#define SCALER_3_Y_RATIO_INV_MASK                                       0x1ffff
#define SCALER_3_Y_RATIO_INV_LSB                                              0

/* ISP_SCALER_4 */
#define SCALER_4_HLPF_COEF_0_MASK                                         0x1ff
#define SCALER_4_HLPF_COEF_0_LSB                                              0
#define SCALER_4_HLPF_COEF_1_MASK                                     0x1ff0000
#define SCALER_4_HLPF_COEF_1_LSB                                             16

/* ISP_SCALER_5 */
#define SCALER_5_HLPF_COEF_2_MASK                                         0x1ff
#define SCALER_5_HLPF_COEF_2_LSB                                              0
#define SCALER_5_HLPF_COEF_3_MASK                                     0x1ff0000
#define SCALER_5_HLPF_COEF_3_LSB                                             16

/* ISP_SCALER_6 */
#define SCALER_6_HLPF_COEF_4_MASK                                         0x1ff
#define SCALER_6_HLPF_COEF_4_LSB                                              0
#define SCALER_6_HLPF_COEF_5_MASK                                     0x1ff0000
#define SCALER_6_HLPF_COEF_5_LSB                                             16

/* ISP_SCALER_7 */
#define SCALER_7_HLPF_COEF_6_MASK                                         0x1ff
#define SCALER_7_HLPF_COEF_6_LSB                                              0
#define SCALER_7_HLPF_COEF_7_MASK                                     0x1ff0000
#define SCALER_7_HLPF_COEF_7_LSB                                             16

/* ISP_SCALER_8 */
#define SCALER_8_VLPF_COEF_0_MASK                                         0x1ff
#define SCALER_8_VLPF_COEF_0_LSB                                              0
#define SCALER_8_VLPF_COEF_1_MASK                                       0x3fe00
#define SCALER_8_VLPF_COEF_1_LSB                                              9
#define SCALER_8_VLPF_COEF_2_MASK                                     0x7fc0000
#define SCALER_8_VLPF_COEF_2_LSB                                             18

/* ISP_SCALER_9 */
#define SCALER_9_SHARP_COEF_0_MASK                                        0x7ff
#define SCALER_9_SHARP_COEF_0_LSB                                             0
#define SCALER_9_SHARP_COEF_1_MASK                                    0x7ff0000
#define SCALER_9_SHARP_COEF_1_LSB                                            16

/* ISP_SCALER_10 */
#define SCALER_10_SHARP_COEF_2_MASK                                       0x7ff
#define SCALER_10_SHARP_COEF_2_LSB                                            0
#define SCALER_10_SHARP_COEF_3_MASK                                   0x7ff0000
#define SCALER_10_SHARP_COEF_3_LSB                                           16

/* ISP_AWB_0 */
#define AWB_0_BLK_NUM_X_MASK                                               0x3f
#define AWB_0_BLK_NUM_X_LSB                                                   0
#define AWB_0_BLK_NUM_Y_MASK                                              0xfc0
#define AWB_0_BLK_NUM_Y_LSB                                                   6

/* ISP_AWB_1 */
#define AWB_1_BLK_WD_MASK                                                 0xfff
#define AWB_1_BLK_WD_LSB                                                      0
#define AWB_1_BLK_HT_MASK                                             0xfff0000
#define AWB_1_BLK_HT_LSB                                                     16

/* ISP_AWB_2 */
#define AWB_2_WIN_START_X_MASK                                            0xfff
#define AWB_2_WIN_START_X_LSB                                                 0
#define AWB_2_WIN_START_Y_MASK                                        0xfff0000
#define AWB_2_WIN_START_Y_LSB                                                16

/* ISP_AWB_3 */
#define AWB_3_BLK_SKIP_X_MASK                                             0xfff
#define AWB_3_BLK_SKIP_X_LSB                                                  0
#define AWB_3_BLK_SKIP_Y_MASK                                         0xfff0000
#define AWB_3_BLK_SKIP_Y_LSB                                                 16

/* ISP_AWB_4 */
#define AWB_4_PRE_OB1_MASK                                                0x3ff
#define AWB_4_PRE_OB1_LSB                                                     0
#define AWB_4_PRE_OB2_MASK                                              0xffc00
#define AWB_4_PRE_OB2_LSB                                                    10
#define AWB_4_PRE_OB3_MASK                                           0x3ff00000
#define AWB_4_PRE_OB3_LSB                                                    20

/* ISP_AWB_5 */
#define AWB_5_PRE_OB4_MASK                                                0x3ff
#define AWB_5_PRE_OB4_LSB                                                     0
#define AWB_5_WHITE_LEVEL_MASK                                          0xffc00
#define AWB_5_WHITE_LEVEL_LSB                                                10
#define AWB_5_BLACK_LEVEL_MASK                                       0x3ff00000
#define AWB_5_BLACK_LEVEL_LSB                                                20

/* ISP_AWB_6 */
#define AWB_6_IR_WHITE_LEVEL_MASK                                         0x3ff
#define AWB_6_IR_WHITE_LEVEL_LSB                                              0
#define AWB_6_IR_BLACK_LEVEL_MASK                                       0xffc00
#define AWB_6_IR_BLACK_LEVEL_LSB                                             10

/* ISP_AWB_7 */
#define AWB_7_PX_MAN_MASK                                                 0x3ff
#define AWB_7_PX_MAN_LSB                                                      0
#define AWB_7_PX_EXP_MASK                                                0x7c00
#define AWB_7_PX_EXP_LSB                                                     10

/* ISP_AE_0 */
#define AE_0_PRE_OB1_MASK                                                 0x3ff
#define AE_0_PRE_OB1_LSB                                                      0
#define AE_0_PRE_OB2_MASK                                               0xffc00
#define AE_0_PRE_OB2_LSB                                                     10

/* ISP_AE_1 */
#define AE_1_PRE_OB3_MASK                                                 0x3ff
#define AE_1_PRE_OB3_LSB                                                      0
#define AE_1_PRE_OB4_MASK                                               0xffc00
#define AE_1_PRE_OB4_LSB                                                     10
#define AE_1_BA_BLK_NUM_X_MASK                                        0x1f00000
#define AE_1_BA_BLK_NUM_X_LSB                                                20
#define AE_1_BA_BLK_NUM_Y_MASK                                       0x3e000000
#define AE_1_BA_BLK_NUM_Y_LSB                                                25

/* ISP_AE_2 */
#define AE_2_BA_WIN_START_X_MASK                                          0xfff
#define AE_2_BA_WIN_START_X_LSB                                               0
#define AE_2_BA_WIN_START_Y_MASK                                      0xfff0000
#define AE_2_BA_WIN_START_Y_LSB                                              16

/* ISP_AE_3 */
#define AE_3_BA_BLK_WD_MASK                                               0xfff
#define AE_3_BA_BLK_WD_LSB                                                    0
#define AE_3_BA_BLK_HT_MASK                                           0xfff0000
#define AE_3_BA_BLK_HT_LSB                                                   16

/* ISP_AE_4 */
#define AE_4_BA_SKIP_X_MASK                                               0xfff
#define AE_4_BA_SKIP_X_LSB                                                    0
#define AE_4_BA_SKIP_Y_MASK                                           0xfff0000
#define AE_4_BA_SKIP_Y_LSB                                                   16

/* ISP_AE_5 */
#define AE_5_BA_PX_MAN_MASK                                               0x3ff
#define AE_5_BA_PX_MAN_LSB                                                    0
#define AE_5_BA_PX_EXP_MASK                                              0xf800
#define AE_5_BA_PX_EXP_LSB                                                   11

/* ISP_AE_6 */
#define AE_6_HIST_WIN_START_X_MASK                                        0xfff
#define AE_6_HIST_WIN_START_X_LSB                                             0
#define AE_6_HIST_WIN_START_Y_MASK                                    0xfff0000
#define AE_6_HIST_WIN_START_Y_LSB                                            16

/* ISP_AE_7 */
#define AE_7_HIST_WIN_WD_MASK                                             0xfff
#define AE_7_HIST_WIN_WD_LSB                                                  0
#define AE_7_HIST_WIN_HT_MASK                                         0xfff0000
#define AE_7_HIST_WIN_HT_LSB                                                 16

/* ISP_ISP_MONITOR0 */
#define ISP_MONITOR0_DBG_INFO_MASK                                   0xffffffff
#define ISP_MONITOR0_DBG_INFO_LSB                                             0

/* ISP_ISP_MONITOR1 */
#define ISP_MONITOR1_DBG_INFO_MASK                                   0xffffffff
#define ISP_MONITOR1_DBG_INFO_LSB                                             0

/* ISP_ISP_MONITOR2 */
#define ISP_MONITOR2_DBG_INFO_MASK                                   0xffffffff
#define ISP_MONITOR2_DBG_INFO_LSB                                             0

/* ISP_ISP_MONITOR3 */
#define ISP_MONITOR3_DBG_INFO_MASK                                   0xffffffff
#define ISP_MONITOR3_DBG_INFO_LSB                                             0

/* ISP_ISP_MONITOR4 */
#define ISP_MONITOR4_DBG_INFO_MASK                                   0xffffffff
#define ISP_MONITOR4_DBG_INFO_LSB                                             0

/* ISP_ISP_MONITOR5 */
#define ISP_MONITOR5_DBG_INFO_MASK                                   0xffffffff
#define ISP_MONITOR5_DBG_INFO_LSB                                             0

/* ISP_ISP_MONITOR6 */
#define ISP_MONITOR6_DBG_INFO_MASK                                   0xffffffff
#define ISP_MONITOR6_DBG_INFO_LSB                                             0

/* ISP_ISP_MONITOR7 */
#define ISP_MONITOR7_DBG_INFO_MASK                                   0xffffffff
#define ISP_MONITOR7_DBG_INFO_LSB                                             0

/* ISP_ISP_MONITOR8 */
#define ISP_MONITOR8_DBG_INFO_MASK                                   0xffffffff
#define ISP_MONITOR8_DBG_INFO_LSB                                             0

/* ISP_ISP_MONITOR9 */
#define ISP_MONITOR9_DBG_INFO_MASK                                   0xffffffff
#define ISP_MONITOR9_DBG_INFO_LSB                                             0

/* ISP_ISP_MONITOR10 */
#define ISP_MONITOR10_DBG_INFO_MASK                                  0xffffffff
#define ISP_MONITOR10_DBG_INFO_LSB                                            0

/* ISP_ISP_MONITOR11 */
#define ISP_MONITOR11_DBG_INFO_MASK                                  0xffffffff
#define ISP_MONITOR11_DBG_INFO_LSB                                            0

/* ISP_ISP_MONITOR12 */
#define ISP_MONITOR12_DBG_INFO_MASK                                  0xffffffff
#define ISP_MONITOR12_DBG_INFO_LSB                                            0

/* ISP_ISP_MONITOR13 */
#define ISP_MONITOR13_DBG_INFO_MASK                                  0xffffffff
#define ISP_MONITOR13_DBG_INFO_LSB                                            0

/* ISP_ISP_MONITOR14 */
#define ISP_MONITOR14_DBG_INFO_MASK                                  0xffffffff
#define ISP_MONITOR14_DBG_INFO_LSB                                            0

/* ISP_ISP_MONITOR15 */
#define ISP_MONITOR15_DBG_INFO_MASK                                  0xffffffff
#define ISP_MONITOR15_DBG_INFO_LSB                                            0

/* ISP_ISP_MONITOR16 */
#define ISP_MONITOR16_DBG_INFO_MASK                                  0xffffffff
#define ISP_MONITOR16_DBG_INFO_LSB                                            0

/* ISP_ISP_MONITOR17 */
#define ISP_MONITOR17_DBG_INFO_MASK                                  0xffffffff
#define ISP_MONITOR17_DBG_INFO_LSB                                            0

/* ISP_ISP_MONITOR18 */
#define ISP_MONITOR18_DBG_INFO_MASK                                  0xffffffff
#define ISP_MONITOR18_DBG_INFO_LSB                                            0

/* ISP_ISP_MONITOR19 */
#define ISP_MONITOR19_DBG_INFO_MASK                                  0xffffffff
#define ISP_MONITOR19_DBG_INFO_LSB                                            0

/* ISP_ISP_MONITOR20 */
#define ISP_MONITOR20_DBG_INFO_MASK                                  0xffffffff
#define ISP_MONITOR20_DBG_INFO_LSB                                            0

/* ISP_ISP_MONITOR21 */
#define ISP_MONITOR21_DBG_INFO_MASK                                  0xffffffff
#define ISP_MONITOR21_DBG_INFO_LSB                                            0

/* ISP_ISP_MONITOR22 */
#define ISP_MONITOR22_DBG_INFO_MASK                                  0xffffffff
#define ISP_MONITOR22_DBG_INFO_LSB                                            0

/* ISP_ISP_MONITOR23 */
#define ISP_MONITOR23_DBG_INFO_MASK                                  0xffffffff
#define ISP_MONITOR23_DBG_INFO_LSB                                            0

/* ISP_ISP_MONITOR24 */
#define ISP_MONITOR24_DBG_INFO_MASK                                  0xffffffff
#define ISP_MONITOR24_DBG_INFO_LSB                                            0

/* ISP_ISP_MONITOR25 */
#define ISP_MONITOR25_DBG_INFO_MASK                                  0xffffffff
#define ISP_MONITOR25_DBG_INFO_LSB                                            0

/* ISP_ISP_MONITOR26 */
#define ISP_MONITOR26_DBG_INFO_MASK                                  0xffffffff
#define ISP_MONITOR26_DBG_INFO_LSB                                            0

/* ISP_ISP_MONITOR27 */
#define ISP_MONITOR27_DBG_INFO_MASK                                  0xffffffff
#define ISP_MONITOR27_DBG_INFO_LSB                                            0

/* ISP_DMA_ADR_EXT0 */
#define DMA_ADR_EXT0_DMA_ADDR_HDR_IN0_EXTEND_MASK                          0xff
#define DMA_ADR_EXT0_DMA_ADDR_HDR_IN0_EXTEND_LSB                              0
#define DMA_ADR_EXT0_DMA_ADDR_HDR_IN1_EXTEND_MASK                        0xff00
#define DMA_ADR_EXT0_DMA_ADDR_HDR_IN1_EXTEND_LSB                              8
#define DMA_ADR_EXT0_DMA_ADDR_FCURVE_SUBIN_EXTEND_MASK                 0xff0000
#define DMA_ADR_EXT0_DMA_ADDR_FCURVE_SUBIN_EXTEND_LSB                        16
#define DMA_ADR_EXT0_DMA_CONFIG_FCURVE_SUBOUT_EXTEND_MASK            0xff000000
#define DMA_ADR_EXT0_DMA_CONFIG_FCURVE_SUBOUT_EXTEND_LSB                     24

/* ISP_DMA_ADR_EXT1 */
#define DMA_ADR_EXT1_DMA_ADDR_MLSC_EXTEND_MASK                             0xff
#define DMA_ADR_EXT1_DMA_ADDR_MLSC_EXTEND_LSB                                 0
#define DMA_ADR_EXT1_DMA_ADDR_WDR_SUBIN_EXTEND_MASK                    0xff0000
#define DMA_ADR_EXT1_DMA_ADDR_WDR_SUBIN_EXTEND_LSB                           16
#define DMA_ADR_EXT1_DMA_ADDR_WDR_SUBOUT_EXTEND_MASK                 0xff000000
#define DMA_ADR_EXT1_DMA_ADDR_WDR_SUBOUT_EXTEND_LSB                          24

/* ISP_DMA_ADR_EXT2 */
#define DMA_ADR_EXT2_DMA_ADDR_LCE_SUBIN_EXTEND_MASK                        0xff
#define DMA_ADR_EXT2_DMA_ADDR_LCE_SUBIN_EXTEND_LSB                            0
#define DMA_ADR_EXT2_DMA_ADDR_LCE_SUBOUT_EXTEND_MASK                     0xff00
#define DMA_ADR_EXT2_DMA_ADDR_LCE_SUBOUT_EXTEND_LSB                           8
#define DMA_ADR_EXT2_DMA_ADDR_CNR_SUBIN_EXTEND_MASK                    0xff0000
#define DMA_ADR_EXT2_DMA_ADDR_CNR_SUBIN_EXTEND_LSB                           16
#define DMA_ADR_EXT2_DMA_ADDR_CNR_SUBOUT_EXTEND_MASK                 0xff000000
#define DMA_ADR_EXT2_DMA_ADDR_CNR_SUBOUT_EXTEND_LSB                          24

/* ISP_DMA_ADR_EXT3 */
#define DMA_ADR_EXT3_DMA_ADDR_NR3D_Y_REF_IN_EXTEND_MASK                    0xff
#define DMA_ADR_EXT3_DMA_ADDR_NR3D_Y_REF_IN_EXTEND_LSB                        0
#define DMA_ADR_EXT3_DMA_ADDR_NR3D_Y_CUR_OUT_EXTEND_MASK                 0xff00
#define DMA_ADR_EXT3_DMA_ADDR_NR3D_Y_CUR_OUT_EXTEND_LSB                       8
#define DMA_ADR_EXT3_DMA_ADDR_NR3D_UV_REF_IN_EXTEND_MASK               0xff0000
#define DMA_ADR_EXT3_DMA_ADDR_NR3D_UV_REF_IN_EXTEND_LSB                      16
#define DMA_ADR_EXT3_DMA_ADDR_NR3D_UV_CUR_OUT_EXTEND_MASK            0xff000000
#define DMA_ADR_EXT3_DMA_ADDR_NR3D_UV_CUR_OUT_EXTEND_LSB                     24

/* ISP_DMA_ADR_EXT4 */
#define DMA_ADR_EXT4_DMA_ADDR_Y_OUT_EXTEND_MASK                            0xff
#define DMA_ADR_EXT4_DMA_ADDR_Y_OUT_EXTEND_LSB                                0
#define DMA_ADR_EXT4_DMA_ADDR_UV_OUT_EXTEND_MASK                         0xff00
#define DMA_ADR_EXT4_DMA_ADDR_UV_OUT_EXTEND_LSB                               8
#define DMA_ADR_EXT4_DMA_ADDR_NR3D_MOTION_REF_IN_EXTEND_MASK           0xff0000
#define DMA_ADR_EXT4_DMA_ADDR_NR3D_MOTION_REF_IN_EXTEND_LSB                  16
#define DMA_ADR_EXT4_DMA_ADDR_NR3D_MOTION_CUR_OUT_EXTEND_MASK        0xff000000
#define DMA_ADR_EXT4_DMA_ADDR_NR3D_MOTION_CUR_OUT_EXTEND_LSB                 24

/* ISP_DMA_ADR_EXT5 */
#define DMA_ADR_EXT5_DMA_ADDR_U_OUT_EXTEND_MASK                            0xff
#define DMA_ADR_EXT5_DMA_ADDR_U_OUT_EXTEND_LSB                                0
#define DMA_ADR_EXT5_DMA_ADDR_V_OUT_EXTEND_MASK                          0xff00
#define DMA_ADR_EXT5_DMA_ADDR_V_OUT_EXTEND_LSB                                8

/* ISP_DMA_ADR_EXT6 */
#define DMA_ADR_EXT6_DMA_ADDR_NR3D_U_REF_IN_EXTEND_MASK                    0xff
#define DMA_ADR_EXT6_DMA_ADDR_NR3D_U_REF_IN_EXTEND_LSB                        0
#define DMA_ADR_EXT6_DMA_ADDR_NR3D_V_REF_IN_EXTEND_MASK                  0xff00
#define DMA_ADR_EXT6_DMA_ADDR_NR3D_V_REF_IN_EXTEND_LSB                        8
#define DMA_ADR_EXT6_DMA_ADDR_NR3D_U_REF_OUT_EXTEND_MASK               0xff0000
#define DMA_ADR_EXT6_DMA_ADDR_NR3D_U_REF_OUT_EXTEND_LSB                      16
#define DMA_ADR_EXT6_DMA_ADDR_NR3D_V_REF_OUT_EXTEND_MASK             0xff000000
#define DMA_ADR_EXT6_DMA_ADDR_NR3D_V_REF_OUT_EXTEND_LSB                      24

/* ISP_ISP_SW0 */
#define ISP_SW0_SW_TMP0_MASK                                         0xffffffff
#define ISP_SW0_SW_TMP0_LSB                                                   0

/* ISP_ISP_SW1 */
#define ISP_SW1_SW_TMP1_MASK                                         0xffffffff
#define ISP_SW1_SW_TMP1_LSB                                                   0

/* ISP_ISP_SW2 */
#define ISP_SW2_SW_TMP2_MASK                                         0xffffffff
#define ISP_SW2_SW_TMP2_LSB                                                   0

/* ISP_ISP_SW3 */
#define ISP_SW3_SW_TMP3_MASK                                         0xffffffff
#define ISP_SW3_SW_TMP3_LSB                                                   0

/* ISP_ISP_SW4 */
#define ISP_SW4_SW_TMP4_MASK                                         0xffffffff
#define ISP_SW4_SW_TMP4_LSB                                                   0

/* ISP_ISP_SW5 */
#define ISP_SW5_SW_TMP5_MASK                                         0xffffffff
#define ISP_SW5_SW_TMP5_LSB                                                   0


#endif /* ISP_REG_H*/

