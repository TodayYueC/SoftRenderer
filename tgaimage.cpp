/**
 * @file tgaimage.cpp
 * @brief TGA 图像类的实现——所有函数的详细注释版本
 *
 * 本文件是整个软渲染器的"像素基础设施"实现。
 * 在学习渲染原理时，可以重点关注 get() 和 set() —— 这是渲染管线的最终输出点。
 * TGA 文件读写的细节（特别是 RLE 压缩）可以暂时跳过，不影响理解核心渲染逻辑。
 */

#include <iostream>
#include <cstring>
#include "tgaimage.h"

// ============================================================================
// 构造函数：创建指定尺寸的空白图像，并用颜色 c 填充
// ============================================================================
/**
 * @param w   宽度（像素）
 * @param h   高度（像素）
 * @param bpp 每像素字节数：1=灰度, 3=RGB, 4=RGBA
 * @param c   填充颜色，默认全零（透明黑）
 *
 * 实现步骤：
 *   1. 初始化成员变量 w, h, bpp
 *   2. 分配 w*h*bpp 字节的连续内存，初始化为 0
 *   3. 遍历所有像素，调用 set() 填充为颜色 c
 *
 * 为什么不直接 memset？
 *   因为 bpp 可能与 c.bytespp 不一致（比如图像是 RGB 但传入的 c 是 RGBA），
 *   set() 内部用 memcpy(c.bgra, bpp) 只拷贝 bpp 个字节，保证了正确性。
 */
TGAImage::TGAImage(const int w, const int h, const int bpp, TGAColor c) : w(w), h(h), bpp(bpp), data(w*h*bpp, 0) {
    for (int j=0; j<h; j++)
        for (int i=0; i<w; i++)
            set(i, j, c);
}

// ============================================================================
// 从 TGA 文件加载图像
// ============================================================================
/**
 * 流程图：
 *
 *   打开文件（二进制模式）
 *        │
 *        ▼
 *   读取 18 字节 TGAHeader
 *        │
 *        ▼
 *   解析宽度、高度、bpp（bitsperpixel>>3）
 *        │
 *        ▼
 *   合法性检查（w>0, h>0, bpp 为 1/3/4）
 *        │
 *        ▼
 *   分配 w*h*bpp 字节像素缓冲
 *        │
 *        ├─ datatypecode==2 或 3 → 直接读取整块像素数据（未压缩）
 *        │
 *        ├─ datatypecode==10 或 11 → 调用 load_rle_data() 解压读取（RLE 压缩）
 *        │
 *        └─ 其他 → 报错，不支持
 *        │
 *        ▼
 *   根据 imagedescriptor 处理翻转：
 *     bit5==0 ↔ 原点在左下角 → 需要垂直翻转
 *     bit4==1 ↔ 水平翻转   → 需要水平翻转
 */
bool TGAImage::read_tga_file(const std::string filename) {
    std::ifstream in;
    in.open(filename, std::ios::binary);  // 必须二进制模式打开，否则换行符会被转换
    if (!in.is_open()) {
        std::cerr << "can't open file " << filename << "\n";
        return false;
    }

    // ---- 第1步：读取文件头（18字节）----
    TGAHeader header;
    // 直接将 18 字节读到结构体中。因为我们用了 #pragma pack(1)，
    // 结构体内存布局与文件格式完全一致，可以整体读取。
    in.read(reinterpret_cast<char *>(&header), sizeof(header));
    if (!in.good()) {
        std::cerr << "an error occured while reading the header\n";
        return false;
    }

    // ---- 第2步：解析文件头 ----
    w   = header.width;
    h   = header.height;
    // bitsperpixel 是每像素的位数（8/24/32），除以 8 得到每像素字节数（1/3/4）
    bpp = header.bitsperpixel>>3;

    // 合法性检查：宽度高度必须为正，bpp 必须是 1(灰度)、3(RGB) 或 4(RGBA)
    if (w<=0 || h<=0 || (bpp!=GRAYSCALE && bpp!=RGB && bpp!=RGBA)) {
        std::cerr << "bad bpp (or width/height) value\n";
        return false;
    }

    // ---- 第3步：读取像素数据 ----
    size_t nbytes = bpp*w*h;  // 总字节数 = 每像素字节数 × 宽 × 高
    data = std::vector<std::uint8_t>(nbytes, 0);  // 分配并初始化为 0

    if (3==header.datatypecode || 2==header.datatypecode) {
        // 未压缩格式（2=RGB未压缩, 3=灰度未压缩）
        // 直接一次性读取所有像素数据
        in.read(reinterpret_cast<char *>(data.data()), nbytes);
        if (!in.good()) {
            std::cerr << "an error occured while reading the data\n";
            return false;
        }
    } else if (10==header.datatypecode||11==header.datatypecode) {
        // RLE 压缩格式（10=RGB压缩, 11=灰度压缩）
        // 需要逐块解压
        if (!load_rle_data(in)) {
            std::cerr << "an error occured while reading the data\n";
            return false;
        }
    } else {
        std::cerr << "unknown file format " << (int)header.datatypecode << "\n";
        return false;
    }

    // ---- 第4步：处理坐标原点方向 ----
    // TGA 格式的 imagedescriptor 字节的 bit5 和 bit4 决定了像素数据的排列方向：
    //
    //   bit5 (0x20):
    //     0 = 原点在左下角，数据从下到上排列（TGA 的传统方向）
    //     1 = 原点在左上角，数据从上到下排列（符合我们平时的习惯）
    //
    //   bit4 (0x10):
    //     0 = 数据从左到右排列（正常）
    //     1 = 数据从右到左排列（水平翻转）
    //
    // 我们希望图像数据统一为"左上角原点、从左到右从上到下"的顺序，
    // 所以：
    //   - 如果 bit5==0（原点在左下角），需要垂直翻转
    //   - 如果 bit4==1（水平翻转），需要水平翻转

    if (!(header.imagedescriptor & 0x20))  // bit5 为 0 → 原点在左下角
        flip_vertically();                  // 翻转后原点就到了左上角
    if (header.imagedescriptor & 0x10)      // bit4 为 1 → 数据是水平翻转的
        flip_horizontally();               // 翻转回来

    std::cerr << w << "x" << h << "/" << bpp*8 << "\n";  // 输出图像信息到 stderr
    return true;
}

// ============================================================================
// RLE 解压：从输入流中读取行程编码的像素数据
// ============================================================================
/**
 * @brief 解压 TGA 的 RLE（Run-Length Encoding）像素数据
 *
 * RLE 是一种简单的无损压缩算法，核心思想是：
 *   将连续相同的像素合并为一个"RLE 块"，不同像素组成"RAW 块"。
 *
 * TGA 的 RLE 格式：
 *   数据由一系列"块"组成，每个块以 1 字节的"块头"开始：
 *
 *   ┌────────────────────────────────────────────────────────┐
 *   │ 块头值 < 128 (0x00~0x7F) → RAW 块                     │
 *   │   后面跟着 (块头值 + 1) 个不重复的像素                  │
 *   │   例：块头=2 → 后面有 3 个不同像素                      │
 *   │                                                        │
 *   │ 块头值 >= 128 (0x80~0xFF) → RLE 块                     │
 *   │   后面跟着 1 个像素，需要重复 (块头值 - 127) 次          │
 *   │   例：块头=130 → 后面 1 个像素重复 3 次                 │
 *   └────────────────────────────────────────────────────────┘
 *
 * 举例：原始像素序列 [A, A, A, B, C, C] 的 RLE 编码为：
 *   [130, A, 129, B, C]    (130→A重复3次, 1→B,C两个不同像素, 等等)
 *   注意这只是示意，实际编码细节更复杂（见 unload_rle_data）
 *
 * @param in 已定位到像素数据起始位置的文件输入流
 * @return true=成功, false=失败或数据损坏
 */
bool TGAImage::load_rle_data(std::ifstream &in) {
    size_t pixelcount = w*h;       // 像素总数
    size_t currentpixel = 0;       // 当前已读取的像素数
    size_t currentbyte  = 0;       // 当前已写入 data 的字节偏移
    TGAColor colorbuffer;          // 临时缓冲区，用于逐像素读取

    do {
        // 读取 1 字节块头
        std::uint8_t chunkheader = 0;
        chunkheader = in.get();
        if (!in.good()) {
            std::cerr << "an error occured while reading the data\n";
            return false;
        }

        if (chunkheader<128) {
            // -------- RAW 块：后面有 (chunkheader + 1) 个不重复的像素 --------
            chunkheader++;  // 数量 = 块头值 + 1（所以最多 128 个像素）
            for (int i=0; i<chunkheader; i++) {
                // 读一个像素（bpp 字节）
                in.read(reinterpret_cast<char *>(colorbuffer.bgra), bpp);
                if (!in.good()) {
                    std::cerr << "an error occured while reading the header\n";
                    return false;
                }
                // 拷贝到 data 数组
                for (int t=0; t<bpp; t++)
                    data[currentbyte++] = colorbuffer.bgra[t];
                currentpixel++;
                if (currentpixel>pixelcount) {  // 防止读取越界
                    std::cerr << "Too many pixels read\n";
                    return false;
                }
            }
        } else {
            // -------- RLE 块：后面 1 个像素重复 (chunkheader - 127) 次 --------
            chunkheader -= 127;  // 重复次数 = 块头值 - 127（所以最多 128 次）
            // 读取 1 个像素
            in.read(reinterpret_cast<char *>(colorbuffer.bgra), bpp);
            if (!in.good()) {
                std::cerr << "an error occured while reading the header\n";
                return false;
            }
            // 将该像素重复写入 data
            for (int i=0; i<chunkheader; i++) {
                for (int t=0; t<bpp; t++)
                    data[currentbyte++] = colorbuffer.bgra[t];
                currentpixel++;
                if (currentpixel>pixelcount) {
                    std::cerr << "Too many pixels read\n";
                    return false;
                }
            }
        }
    } while (currentpixel < pixelcount);  // 直到读完所有像素
    return true;
}

// ============================================================================
// 保存图像到 TGA 文件
// ============================================================================
/**
 * @brief 将当前图像保存为 TGA 文件
 *
 * 文件结构：
 *   ┌──────────────────┐
 *   │ TGA Header(18B)  │  ← 文件头，描述图像元数据
 *   ├──────────────────┤
 *   │ Pixel Data       │  ← 像素数据（未压缩或 RLE 压缩）
 *   ├──────────────────┤
 *   │ Developer Area(4B)│  ← 空的开发者区域（全零）
 *   ├──────────────────┤
 *   │ Extension Area(4B) │  ← 空的扩展区域（全零）
 *   ├──────────────────┤
 *   │ Footer(18B)       │  ← "TRUEVISION-XFILE." 标识符
 *   └──────────────────┘
 *
 * TGA 2.0 规范要求文件末尾有 "TRUEVISION-XFILE.\0"（18 字节），
 * 这让图像查看器能识别文件是否为 TGA 2.0 格式。
 * 虽然本项目的图像查看器不强制要求这个尾部，但写入它是好习惯。
 *
 * @param filename 输出文件路径
 * @param vflip  控制 y 轴方向：
 *               true  → 写入时标识为"左上角原点"（imagedescriptor=0x00），
 *                       数据按从上到下排列，大多数查看器直接显示
 *               false → 写入时标识为"左下角原点"（imagedescriptor=0x20），
 *                       数据按从下到上排列（TGA 传统方向）
 * @param rle   是否使用 RLE 压缩：
 *               true  → 压缩存储，文件更小
 *               false → 不压缩，文件更大但写入更简单快速（调试推荐）
 */
bool TGAImage::write_tga_file(const std::string filename, const bool vflip, const bool rle) const {
    // TGA 2.0 规范要求的尾部标识
    constexpr std::uint8_t developer_area_ref[4] = {0, 0, 0, 0};
    constexpr std::uint8_t extension_area_ref[4] = {0, 0, 0, 0};
    constexpr std::uint8_t footer[18] = {'T','R','U','E','V','I','S','I','O','N','-','X','F','I','L','E','.','\0'};

    std::ofstream out;
    out.open(filename, std::ios::binary);
    if (!out.is_open()) {
        std::cerr << "can't open file " << filename << "\n";
        return false;
    }

    // ---- 第1步：构造文件头 ----
    TGAHeader header = {};
    header.bitsperpixel = bpp<<3;        // 字节数 × 8 = 位数（如 bpp=3 → 24位）
    header.width  = w;
    header.height = h;
    // 根据 bpp 和 rle 参数决定 datatypecode：
    //   灰度: 未压缩=3, RLE=11
    //   RGB:  未压缩=2, RLE=10
    header.datatypecode = (bpp==GRAYSCALE ? (rle?11:3) : (rle?10:2));
    // vflip 参数控制 y 轴方向：
    //   vflip=true  → 0x00（左上角原点，不翻转，符合屏幕坐标系）
    //   vflip=false → 0x20（左下角原点，TGA 传统方向）
    header.imagedescriptor = vflip ? 0x00 : 0x20;

    // ---- 第2步：写入文件头 ----
    out.write(reinterpret_cast<const char *>(&header), sizeof(header));
    if (!out.good()) goto err;

    // ---- 第3步：写入像素数据 ----
    if (!rle) {
        // 未压缩：直接写入整个 data 数组
        out.write(reinterpret_cast<const char *>(data.data()), w*h*bpp);
        if (!out.good()) goto err;
    } else if (!unload_rle_data(out)) goto err;

    // ---- 第4步：写入 TGA 2.0 尾部（空区域 + 标识符）----
    out.write(reinterpret_cast<const char *>(developer_area_ref), sizeof(developer_area_ref));
    if (!out.good()) goto err;
    out.write(reinterpret_cast<const char *>(extension_area_ref), sizeof(extension_area_ref));
    if (!out.good()) goto err;
    out.write(reinterpret_cast<const char *>(footer), sizeof(footer));
    if (!out.good()) goto err;
    return true;

err:
    std::cerr << "can't dump the tga file\n";
    return false;
}

// ============================================================================
// RLE 压缩：将像素数据以行程编码格式写入输出流
// ============================================================================
/**
 * @brief 将图像的像素数据进行 RLE 压缩并写入文件
 *
 * 编码策略：
 *   扫描像素序列，动态判断每个块应该是 RAW（不重复像素）还是 RLE（重复像素）：
 *
 *   - 如果连续像素颜色相同 → 组成 RLE 块（1字节块头 + 1个像素数据）
 *   - 如果连续像素颜色不同 → 组成 RAW 块（1字节块头 + N个像素数据）
 *
 *   RAW 块头：< 128，表示后面有 (块头+1) 个不重复像素
 *   RLE 块头：≥ 128，表示后面 1 个像素重复 (块头-127) 次
 *
 *   最大块长度为 128 个像素（TGA 规范限制）。
 *
 * @param out 已打开的文件输出流（头部已写入，指针在像素数据位置）
 * @return true=成功, false=写入失败
 */
bool TGAImage::unload_rle_data(std::ofstream &out) const {
    const std::uint8_t max_chunk_length = 128;  // TGA 规范限制：单个块最多 128 个像素
    size_t npixels = w*h;     // 像素总数
    size_t curpix = 0;        // 当前处理的像素索引

    while (curpix<npixels) {
        size_t chunkstart = curpix*bpp;  // 当前块在 data 中的起始字节偏移
        size_t curbyte = curpix*bpp;     // 当前比较位置的字节偏移
        std::uint8_t run_length = 1;     // 当前块的像素长度（至少 1）
        bool raw = true;                  // 标记当前块是否为 RAW（不重复）类型

        // 扫描后续像素，确定块的类型和长度
        while (curpix+run_length<npixels && run_length<max_chunk_length) {
            // 检查下一个像素是否与当前像素相同
            bool succ_eq = true;  // "successor equal"：后继像素是否等于当前像素
            for (int t=0; succ_eq && t<bpp; t++)
                succ_eq = (data[curbyte+t]==data[curbyte+t+bpp]);
            curbyte += bpp;  // 移动到下一个像素

            if (1==run_length)
                raw = !succ_eq;  // 第一个后续像素：如果相同则 RLE，不同则 RAW

            if (raw && succ_eq) {
                // 当前是 RAW 块，但遇到了相同像素 → 结束 RAW 块（不含此相同像素）
                run_length--;
                break;
            }
            if (!raw && !succ_eq)
                // 当前是 RLE 块，但遇到了不同像素 → 结束 RLE 块
                break;

            run_length++;
        }

        curpix += run_length;  // 前进已处理的像素数

        // 写入块头：
        //   RAW 块：块头值 = run_length - 1（范围 0~127）
        //   RLE 块：块头值 = run_length + 127（范围 128~255）
        out.put(raw ? run_length-1 : run_length+127);
        if (!out.good()) return false;

        // 写入像素数据：
        //   RAW 块：写入 run_length 个像素（run_length * bpp 字节）
        //   RLE 块：只写入 1 个像素（bpp 字节），解码时会自动重复
        out.write(reinterpret_cast<const char *>(data.data()+chunkstart), (raw?run_length*bpp:bpp));
        if (!out.good()) return false;
    }
    return true;
}

// ============================================================================
// 读取像素颜色
// ============================================================================
/**
 * @brief 从图像中读取 (x, y) 位置的像素颜色
 *
 * 这是帧缓冲和纹理采样的核心操作。
 *
 * 内存寻址计算：
 *   offset = (x + y * width) * bytes_per_pixel
 *
 *   示例（RGB 图像，width=3）：
 *   像素(1,1) 的偏移 = (1 + 1*3) * 3 = 12
 *   在 data 数组中：[R0,G0,B0, R1,G1,B1, R2,G2,B2, | R3,G3,B3, R4,G4,B4, R5,G5,B5, | ...]
 *                                       ↑ 像素(0,1)      ↑ 像素(1,1) = 偏移12
 *
 * 越界保护：
 *   如果 data 为空、或坐标超出 [0, width) × [0, height)，返回全零颜色。
 *   这相当于 "clamp to edge" 以外的部分返回透明黑色，
 *   在光栅化时非常方便——不需要在调用前检查坐标范围。
 *
 * 注意倒序拷贝（for (int i=bpp; i--; ...)）：
 *   这是作者的编码风格，从 bpp-1 拷贝到 0，效果和正序一样，
 *   但少了 i>=0 的判断（i-- 在 0 时为假，自动终止）。
 *   作者注释说这是"向老派游戏程序员致敬"，现代编译器下无性能差异。
 */
TGAColor TGAImage::get(const int x, const int y) const {
    if (!data.size() || x<0 || y<0 || x>=w || y>=h) return {};  // 越界保护
    TGAColor ret = {0, 0, 0, 0, bpp};  // 初始化返回值，bytespp 设为当前图像的 bpp
    const std::uint8_t *p = data.data()+(x+y*w)*bpp;  // 计算 (x,y) 像素在数组中的指针
    for (int i=bpp; i--; ret.bgra[i] = p[i]);  // 从 pixel buffer 拷贝 bpp 个字节到返回值
    return ret;
}

// ============================================================================
// 写入像素颜色
// ============================================================================
/**
 * @brief 在图像的 (x, y) 位置设置像素颜色
 *
 * 这是光栅化器最关键的输出操作——每渲染一个像素就调用一次。
 * 整个渲染管线的最终目的就是算出 (x, y, color)，然后调用此函数写进去。
 *
 * 实现非常简单：计算偏移后用 memcpy 拷贝 bpp 字节。
 *
 * 越界坐标被静默忽略（return 而不报错），这在渲染中很常见：
 *   光栅化时包围盒可能超出屏幕范围，逐像素检查太慢，
 *   所以在 set() 里做边界检查比在调用方更高效。
 *
 * 性能注释：
 *   此函数是渲染管线的热点（hotspot）。对于一个 800×800 的图像，
 *   三角形光栅化每帧可能调用此函数数十万次。
 *   用 memcpy 而非逐字节赋值是有意为之——
 *   对于 bpp=3 或 4 的情况，编译器可以将 memcpy 优化为单条 mov 指令。
 */
inline void TGAImage::set(int x, int y, const TGAColor &c) {
    if (!data.size() || x<0 || y<0 || x>=w || y>=h) return;  // 越界保护
    memcpy(data.data()+(x+y*w)*bpp, c.bgra, bpp);  // 直接拷贝 bpp 字节
}

// ============================================================================
// 水平翻转图像（左右镜像）
// ============================================================================
/**
 * @brief 将图像进行水平翻转（沿垂直轴镜像）
 *
 * 效果：像素 (x, y) ↔ 像素 (w-1-x, y)
 *
 *   翻转前:  A B C        翻转后:  C B A
 *            D E F                 F E D
 *
 * 实现方式：对每一行的每一对对称像素 (i, w-1-i)，逐字节交换。
 * 时间复杂度：O(w * h * bpp / 2)
 *
 * 使用场景：当 TGA 文件的 imagedescriptor bit4 为 1 时，
 *   表示像素数据是从右到左排列的，需要水平翻转才能正确显示。
 */
void TGAImage::flip_horizontally() {
    for (int i=0; i<w/2; i++)           // 遍历左半部分的列
        for (int j=0; j<h; j++)         // 遍历所有行
            for (int b=0; b<bpp; b++)   // 遍历所有颜色通道
                std::swap(data[(i+j*w)*bpp+b], data[(w-1-i+j*w)*bpp+b]);
}

// ============================================================================
// 垂直翻转图像（上下镜像）
// ============================================================================
/**
 * @brief 将图像进行垂直翻转（沿水平轴镜像）
 *
 * 效果：像素 (x, y) ↔ 像素 (x, h-1-y)
 *
 *   翻转前:  A B C        翻转后:  D E F
 *            D E F                 A B C
 *
 * 实现方式：对每一列的每一对对称行 (j, h-1-j)，逐字节交换。
 * 时间复杂度：O(w * h * bpp / 2)
 *
 * 使用场景：最常用的翻转操作。因为 TGA 格式的坐标原点在左下角，
 *   而几乎所有图像查看器的坐标系原点在左上角，
 *   所以读取 TGA 文件时几乎总是需要垂直翻转。
 *
 *   保存时也需要考虑：
 *   - write_tga_file(vflip=true) → 写入时标记为左上角原点（0x00），不需要翻转
 *   - write_tga_file(vflip=false) → 写入时标记为左下角原点（0x20），需要翻转
 */
void TGAImage::flip_vertically() {
    for (int i=0; i<w; i++)             // 遍历所有列
        for (int j=0; j<h/2; j++)       // 遍历上半部分的行
            for (int b=0; b<bpp; b++)   // 遍历所有颜色通道
                std::swap(data[(i+j*w)*bpp+b], data[(i+(h-1-j)*w)*bpp+b]);
}

// ============================================================================
// 图像尺寸查询
// ============================================================================
int TGAImage::width() const {
    return w;
}

int TGAImage::height() const {
    return h;
}