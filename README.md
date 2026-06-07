# Neko BASIC

A simple BASIC interpreter and REPL written in C.

## Features

- **REPL Interface**: Interactive environment for writing and running BASIC programs.
- **Memory Management**: Configurable memory pool for program and variable storage.
- **BASIC Dialect**: Supports a variety of standard BASIC keywords and functions.
- **Cross-Platform**: Built with CMake, supporting both Linux and Windows.

## Supported Keywords

### Statements
`PRINT`, `LET`, `IF`, `THEN`, `ELSE`, `GOTO`, `GOSUB`, `RETURN`, `FOR`, `TO`, `STEP`, `NEXT`, `WHILE`, `WEND`, `INPUT`, `DIM`, `END`, `STOP`, `REM`

### Graphics
`SCREEN`, `CIRCLE`, `LINE`, `RECT`, `COLOR`, `CLEAR`, `FLIP`, `POINT`, `TEXT`, `SPRITE`, `FILLED`, `CLOSE`

### Commands
`LIST`, `RUN`, `NEW`, `LOAD`, `SAVE`

### Functions & Operators
- **Arithmetic**: `+`, `-`, `*`, `/`, `^`, `MOD`
- **Logical**: `AND`, `OR`, `NOT`
- **Comparison**: `=`, `<>`, `<`, `>`, `<=`, `>=`
- **Mathematical**: `SQR`, `INT`, `ABS`, `RND`, `SIN`, `COS`, `TAN`, `LOG`, `EXP`, `ATN`, `SGN`
- **Strings**: `LEN`, `MID$`, `LEFT$`, `RIGHT$`, `STR$`, `VAL`, `CHR$`, `ASC`

## Building

### Prerequisites
- C compiler (supporting C23)
- CMake 4.0 or higher

### Build Instructions
```bash
mkdir build
cd build
cmake .. # Add -DNEKO_BASIC_GRAPHICS=OFF to disable graphics
cmake --build .
```

## Usage

Run the executable to start the REPL:
```bash
./neko_basic
```

### Command Line Arguments
- `-m`, `--mem <size>`: Specify the memory pool size in MiB. If not provided, the program will prompt for it on startup.

## Example Program

```basic
10 PRINT "Hello, Neko BASIC!"
20 FOR I = 1 TO 5
30   PRINT "Iteration: "; I
40 NEXT I
50 END
```

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
