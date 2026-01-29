
## 状态机
- **一句话**:把“复杂流程”拆成若干个互斥的阶段（状态），每次根据当前状态 + 输入事件决定：做什么、跳到哪个状态。
- **目的**:项目里提到“状态机”，一般是为了让解析/处理流程可控、可维护、可逐步推进
- **三个要素**:  
  - **A.状态（State）**
比如 HTTP 解析常见状态：
REQUEST_LINE：解析请求行（GET /index.html HTTP/1.1）
HEADERS：解析请求头（Host: ...、Connection: ...）
CONTENT：解析请求体（POST 才有）
  - **B.事件/输入（Event/Input）**
这里的输入通常是：
新收到的字节流
从字节流里切出来的一行（\r\n 结尾）
  - **C. 转移（Transition）**
根据当前状态和输入结果，决定：
留在当前状态继续吃数据
切换到下一个状态
返回成功/失败/需要更多数据
- **项目运用**
  - **从状态机**  
    从 buffer 里“切行”的状态机：从字节流切出一行
    通常叫 parse_line()，它处理的是字符级别的：
    找到 \r\n
    把一行变成 C 字符串（中间塞 \0）
    返回：LINE_OK / LINE_BAD / LINE_OPEN（OPEN=行还没接收完整）
    这是一层“小状态机”，它的状态类似：
    目前读到哪里了
    遇到 \r 了没、下一位是不是 \n  
  - **主状态机**  
    处理请求语义的主状态机：请求行→头→体
通常叫 process_read()，它会循环调用 parse_line()：
在 REQUEST_LINE 状态：拿到一行就解析方法/URL/版本
解析成功 → 切到 HEADERS
在 HEADERS：一行行读头
遇到空行（\r\n）→ 若无 body 则请求结束；有 body 则切到 CONTENT
为什么要两层？
因为“切行”是纯文本切割规则；“请求行/头/体”是 HTTP 语义规则。拆开后更清晰。
- **demo代码**
  ```cpp
  enum CHECK_STATE { REQUEST_LINE, HEADER, CONTENT };
    CHECK_STATE state = REQUEST_LINE;
    while (true) {
        LINE_STATUS ls = parse_line();        // 从buffer切一行
        if (ls == LINE_OPEN) return NEED_MORE;
        if (ls == LINE_BAD)  return BAD_REQUEST;

        char* line = get_line();
        if (state == REQUEST_LINE) {
            if (!parse_request_line(line)) return BAD_REQUEST;
            state = HEADER;
        } else if (state == HEADER) {
            if (line_is_empty(line)) {
                if (content_length == 0) return GET_REQUEST; // 完整请求
                state = CONTENT;
            } else {
                parse_header(line);
            }
        } else if (state == CONTENT) {
            if (body_not_complete_yet()) return NEED_MORE;
            return GET_REQUEST;
        }
    }   
    ```
    
  


