# mini-riscv-os -- 一步一步自製 RISC-V 處理器上的嵌入式作業系統

## 簡介

mini-riscv-os 是仿照 [jserv](https://github.com/jserv) 的 [mini-arm-os](https://github.com/jserv/mini-arm-os) 所做出來的專案。

陳鍾誠 [ccckmit](https://github.com/ccckmit) 參考 mini-arm-os 專案，改寫成 RISC-V 處理器的 mini-riscv-os，然後在 Windows 10 作業系統中編譯並執行 。

## 安裝指引

目前只在 windows 10 平台中建置並測試，請安裝 git bash 與 [FreedomStudio](https://www.sifive.com/software)。

下載 FreedomStudio 的 windows 版後，請將 `riscv64-unknown-elf-gcc/bin` 和 `riscv-qemu/bin` 加入系統路徑 PATH 設到 中。

舉例而言，在我的電腦中，我將下列兩個資料夾加入 PATH 中。

```
D:\install\FreedomStudio-2020-06-3-win64\SiFive\riscv64-unknown-elf-gcc-8.3.0-2020.04.1\bin

D:\install\FreedomStudio-2020-06-3-win64\SiFive\riscv-qemu-4.2.0-2020.04.0\bin
```

然後請用 git bash 來建置編譯本課程的專案。我是在 vscode (Visual Studio Code) 中開終端機 Select Default Shell 設成 bash ，然後編譯這些專案的。

## 自製 RISC-V 嵌入式作業系統課程

* [01-HelloOs](01-HelloOs.md)
  - 使用 UART 印出 `"Hello OS!"` 訊息
* [02-ContextSwitch](02-ContextSwitch.md)
  - 從 OS 切到第一個使用者行程 (user task)
* [03-MultiTasking](03-MultiTasking.md)
  - 可以在作業系統與兩個行程之間切換 (協同式多工，非強制切換 Non Preemptive)
* [04-TimerInterrupt](04-TimerInterrupt.md)
  - 示範如何引發時間中斷 (大約每秒一次)
* [05-Preemptive](05-Preemptive.md)
  - 透過時間中斷強制切換的 Preemptive 作業系統。

## 建置方法

* 先切到對應資料夾，例如 `cd 01-HelloOs`，然後執行下列指令。

```
make
make qemu
```

如果你沒有 make，請嘗試下列方式：

1. 用 choco 安裝 make
    * [choco install make](https://chocolatey.org/packages/make)
    * 當然在此之前得先 [安裝好 choco](https://chocolatey.org/install)
2. 或者下載 CodeBlocks ，然後加入 `CodeBlocks\MinGW\bin` 到 PATH 中。
    * 例如我的系統裡 make 是在 `C:\Program Files (x86)\CodeBlocks\MinGW\bin`

## 授權聲明

`mini-riscv-os` 在遵守 BSD 授權協議的情況下，可以被自由的複製、散布與修改，詳細授權聲明請看 [LICENSE](../../LICENSE)。

## 參考文獻

* [Adventures in RISC-V](https://matrix89.github.io/writes/writes/experiments-in-riscv/)
* [Xv6, a simple Unix-like teaching operating system](https://pdos.csail.mit.edu/6.828/2020/xv6.html)
* [Basics of programming a UART](https://www.activexperts.com/serial-port-component/tutorials/uart/)