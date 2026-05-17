/**
 * @file tgaimage.h
 * @brief TGA 图像文件的读写接口定义
 *
 * TGA（Truevision TGA）是一种极其简单的图像格式，非常适合教学目的：
 *   - 没有复杂的压缩算法（可选 RLE）
 *   - 没有色彩空间转换
 *   - 没有专利限制
 *   - 二进制结构简单：文件头(18字节) + 像素数据 + 可选尾部
 *
 * 在本项目中，TGAImage 同时充当两个角色：
 *   1. 帧缓冲（Framebuffer）：渲染器的输出画布，光栅化器往里写像素
 *   2. 纹理（Texture）：从 .tga 文件加载漫反射贴图、法线贴图、高光贴图等
 */

#pragma once
#include <cstdint>
#include <fstream>
#include <vector>

/**
 * @brief TGA 文件头结构（18 字节）
 *
 * TGA 文件的起始 18 字节，严格按格式规范排列。
 * 使用 #pragma pack(push,1) 确保结构体紧凑排列（无内存对齐填充），
 * 这样 sizeof(TGAHeader) == 18，可以直接整体读写。
 *
 * 为什么需要 pack(1)？
 *   默认情况下，编译器可能会在结构体成员之间插入填充字节（padding），
 *   以保证内存对齐（例如 uint16_t 可能被对齐到 2 字节边界）。
 *   但文件是按字节顺序紧凑存储的，如果结构体有填充，读取就会错位。
 *   pack(1) 强制编译器取消对齐，确保结构体内存布局与文件格式完全一致。
 *
 * TGA 文件头各字段含义（v1.0 规范）：
 *   ┌─────────────────┬──────┬──────────────────────────────────┐
 *   │ 字段            │ 偏移 │ 说明                             │
 *   ├─────────────────┼──────┼──────────────────────────────────┤
 *   │ idlength        │  0   │ 图像 ID 字段长度（通常为 0）     │
 *   │ colormaptype    │  1   │ 调色板类型（0=无，1=有）         │
 *   │ datatypecode    │  2   │ 图像数据类型（见下方说明）       │
 *   │ colormaporigin  │  3   │ 调色板起始索引                   │
 *   │ colormaplength  │  5   │ 调色板条目数                     │
 *   │ colormapdepth   │  7   │ 调色板每条目位数                 │
 *   │ x_origin        │  8   │ 图像左下角 X 坐标                │
 *   │ y_origin        │ 10   │ 图像左下角 Y 坐标                │
 *   │ width           │ 12   │ 图像宽度（像素）                 │
 *   │ height          │ 14   │ 图像高度（像素）                 │
 *   │ bitsperpixel    │ 16   │ 每像素位数（8/16/24/32）        │
 *   │ imagedescriptor │ 17   │ 图像描述符字节                   │
 *   └─────────────────┴──────┴──────────────────────────────────┘
 *
 * datatypecode 取值：
 *   0  - 无图像数据
 *   1  - 未压缩，调色板索引
 *   2  - 未压缩，真彩色（RGB）        ← 本项目主要用这个
 *   3  - 未压缩，灰度图               ← 本项目也支持
 *   9  - RLE 压缩，调色板索引
 *   10 - RLE 压缩，真彩色（RGB）      ← 本项目也支持
 *   11 - RLE 压缩，灰度图             ← 本项目也支持
 *
 * imagedescriptor 的重要位：
 *   bit5 (0x20) = 1 → 原点在左上角（与我们习惯一致）
 *   bit5 (0x20) = 0 → 原点在左下角（TGA 默认，需要垂直翻转）
 *   bit4 (0x10) = 1 → 水平翻转
 */
#pragma pack(push,1)
struct TGAHeader {
    std::uint8_t  idlength = 0;        // 图像 ID 字段的长度，通常为 0（无 ID）
    std::uint8_t  colormaptype = 0;    // 调色板类型：0=无调色板，1=有调色板
    std::uint8_t  datatypecode = 0;   // 图像数据类型编码（2/3/10/11 为本项目支持）
    std::uint16_t colormaporigin = 0;  // 调色板起始索引（本项目不用调色板）
    std::uint16_t colormaplength = 0;  // 调色板条目数量
    std::uint8_t  colormapdepth = 0;   // 调色板每个条目的位数
    std::uint16_t x_origin = 0;       // 图像左下角 X 坐标（通常为 0）
    std::uint16_t y_origin = 0;       // 图像左下角 Y 坐标（通常为 0）
    std::uint16_t width = 0;          // 图像宽度（像素）
    std::uint16_t height = 0;         // 图像高度（像素）
    std::uint8_t  bitsperpixel = 0;   // 每像素位数：8(灰度)/24(RGB)/32(RGBA)
    std::uint8_t  imagedescriptor = 0; // 描述符：bit5=垂直方向, bit4=水平方向
};
#pragma pack(pop)

/**
 * @brief 表示一个像素颜色
 *
 * 存储格式为 BGRA（蓝-绿-红-透明），而不是常见的 RGBA。
 * 这是 TGA 格式的历史遗留——TGA 规范定义的像素顺序就是 BGR(A)。
 *
 * 所以：
 *   白色 = {255, 255, 255, 255}  （B=255, G=255, R=255, A=255）
 *   红色 = {  0,   0, 255, 255}  （B=0,   G=0,   R=255, A=255）
 *   绿色 = {  0, 255,   0, 255}  （B=0,   G=255, R=0,   A=255）
 *   蓝色 = {255, 128,  64, 255}  原代码中 blue 的定义，注意是 BGRA！
 *
 * bytespp（bytes per pixel）：每像素占用的字节数
 *   1 = 灰度图（只有 bgra[0] 有效）
 *   3 = RGB（使用 bgra[0..2]）
 *   4 = RGBA（使用 bgra[0..3]）
 */
struct TGAColor {
    std::uint8_t bgra[4] = {0,0,0,0}; // 像素数据，注意是 BGRA 顺序而非 RGBA
    std::uint8_t bytespp = 4;         // 每像素字节数（1/3/4），用于标识颜色格式

    /**
     * @brief 按索引访问颜色通道
     * @param i 通道索引：0=B, 1=G, 2=R, 3=A
     * @return 对应通道的字节引用
     *
     * 用法示例：
     *   TGAColor c = {0, 0, 255, 255};  // 红色
     *   c[0]  // 返回 0   (Blue)
     *   c[2]  // 返回 255 (Red)
     */
    std::uint8_t& operator[](const int i) { return bgra[i]; }
    const std::uint8_t& operator[](const int i) const { return bgra[i]; }
};

/**
 * @brief TGA 图像类，提供图像的创建、读写、像素操作等核心功能
 *
 * 这是整个软渲染器的图像基础设施，充当帧缓冲和纹理容器。
 * 内部使用一维数组存储像素数据，按行优先（row-major）排列：
 *
 *   内存布局示意（3×2 的 RGB 图像）：
 *   ┌──────────┬──────────┬──────────┬──────────┬──────────┬──────────┐
 *   │ pixel(0,0) │ pixel(1,0) │ pixel(2,0) │ pixel(0,1) │ pixel(1,1) │ pixel(2,1) │
 *   └──────────┴──────────┴──────────┴──────────┴──────────┴──────────┘
 *
 *   像素 (x, y) 在数组中的偏移量 = (x + y * width) * bytes_per_pixel
 *
 * 设计决策：
 *   - 不使用二维数组，避免额外的内存分配开销
 *   - 不使用智能指针，直接用 std::vector<uint8_t> 管理内存
 *   - 不做任何图像处理（无滤波、无色彩空间转换）
 *   - 越界访问时不崩溃，而是静默返回/忽略（方便渲染时不用四面检查）
 */
struct TGAImage {
    /** 支持的像素格式 */
    enum Format { GRAYSCALE=1, RGB=3, RGBA=4 };

    TGAImage() = default;
    /**
     * @brief 创建指定尺寸和格式的图像，并用指定颜色填充
     * @param w     宽度（像素）
     * @param h     高度（像素）
     * @param bpp   每像素字节数（1=灰度, 3=RGB, 4=RGBA）
     * @param c     填充颜色，默认为全透明黑色 {0,0,0,0}
     */
    TGAImage(const int w, const int h, const int bpp, TGAColor c = {});

    /**
     * @brief 从文件加载 TGA 图像
     * @param filename 文件路径
     * @return true=加载成功, false=失败
     *
     * 支持的图像类型：
     *   - 未压缩的 RGB/灰度（datatypecode = 2/3）
     *   - RLE 压缩的 RGB/灰度（datatypecode = 10/11）
     */
    bool  read_tga_file(const std::string filename);

    /**
     * @brief 保存图像到 TGA 文件
     * @param filename 输出文件路径
     * @param vflip  是否进行垂直翻转。默认 true：
     *                true  → 写入时原点在左上角（符合常见图像格式习惯）
     *                false → 写入时原点在左下角（TGA 原始格式）
     * @param rle    是否使用 RLE 压缩。默认 true：
     *                true  → 文件更小
     *                false → 文件更大但写入更快，调试时推荐
     * @return true=保存成功, false=失败
     */
    bool write_tga_file(const std::string filename, const bool vflip=true, const bool rle=true) const;

    /** 水平翻转图像（左右镜像） */
    void flip_horizontally();
    /** 垂直翻转图像（上下镜像），读取 TGA 时常用，因为 TGA 原点在左下角 */
    void flip_vertically();

    /**
     * @brief 读取指定位置的像素颜色
     * @param x 横坐标（0 到 width-1）
     * @param y 纵坐标（0 到 height-1）
     * @return 像素颜色，越界时返回全零 {0,0,0,0}
     *
     * 这是渲染器中采样纹理的核心操作，每帧可能调用数十万次。
     */
    TGAColor get(const int x, const int y) const;

    /**
     * @brief 设置指定位置的像素颜色
     * @param x 横坐标
     * @param y 纵坐标
     * @param c 要设置的颜色
     *
     * 这是渲染器中写入帧缓冲的核心操作，光栅化器每算出一个像素就调用此函数。
     * 越界坐标会被静默忽略。
     */
    void set(const int x, const int y, const TGAColor &c);

    /** @return 图像宽度（像素） */
    int width()  const;
    /** @return 图像高度（像素） */
    int height() const;

private:
    /**
     * @brief 从输入流加载 RLE 压缩的像素数据
     * @param in 已打开的文件输入流（头部已读取，指针在像素数据起始位置）
     * @return true=成功, false=失败
     *
     * RLE 解压算法：
     *   读取一个"块头"字节：
     *     - 如果 < 128：后面紧跟 (块头+1) 个不重复的像素（RAW 块）
     *     - 如果 >= 128：后面紧跟 1 个像素，重复 (块头-127) 次（RLE 块）
     *   然后读取下一个块头，直到所有像素读完。
     */
    bool   load_rle_data(std::ifstream &in);

    /**
     * @brief 将像素数据以 RLE 压缩格式写入输出流
     * @param out 已打开的文件输出流
     * @return true=成功, false=失败
     *
     * RLE 压缩算法会扫描像素序列，将连续相同的像素打包成 RLE 块，
     * 将不相同的像素打包成 RAW 块，以减小文件体积。
     */
    bool unload_rle_data(std::ofstream &out) const;

    int w = 0;                              // 图像宽度（像素）
    int h = 0;                              // 图像高度（像素）
    std::uint8_t bpp = 0;                   // 每像素字节数（1=灰度, 3=RGB, 4=RGBA）
    std::vector<std::uint8_t> data = {};    // 像素数据，一维数组，按行优先排列
                                            // 像素 (x,y) 的偏移 = (x + y*w) * bpp
};