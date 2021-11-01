OUTPUT_ARCH( "riscv" ) /* 處理器架構為 RISCV */

ENTRY( _start ) /* 進入點從 _start 開始執行 */

MEMORY
{
  ram   (wxa!ri) : ORIGIN = 0x80000000, LENGTH = 128M /* RAM 從 0x80000000 開始，共 128M BYTES */
}

PHDRS
{
  text PT_LOAD; /* text 段要載入*/ /* PT_LOAD (1) Indicates that this program header describes a segment to be loaded from the file. 參考 -- https://ftp.gnu.org/old-gnu/Manuals/ld-2.9.1/html_node/ld_23.html */
  data PT_LOAD; /* data 段要載入*/
  bss PT_LOAD;  /* bss 段要載入*/
}

SECTIONS
{
  .text : {
    PROVIDE(_text_start = .); /* 設定 _text_start 為 .text 段開頭位址。 */
    *(.text.init) *(.text .text.*) /* 將所有程式碼放在這段。 */
    PROVIDE(_text_end = .); /* 設定 _text_end 為 .text 段開頭位址。 */
  } >ram AT>ram :text

  .rodata : { /* 唯讀資料段 */
    PROVIDE(_rodata_start = .);
    *(.rodata .rodata.*)
    PROVIDE(_rodata_end = .);
  } >ram AT>ram :text

  .data : { /* 資料段 */
    . = ALIGN(4096);
    PROVIDE(_data_start = .);
    *(.sdata .sdata.*) *(.data .data.*)
    PROVIDE(_data_end = .);
  } >ram AT>ram :data

  .bss :{ /* 未初始化資料段 */
    PROVIDE(_bss_start = .);
    *(.sbss .sbss.*) *(.bss .bss.*)
    PROVIDE(_bss_end = .);
  } >ram AT>ram :bss

  PROVIDE(_memory_start = ORIGIN(ram));
  PROVIDE(_memory_end = ORIGIN(ram) + LENGTH(ram));
}
