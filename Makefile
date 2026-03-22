default:
# ============================================
# СБОРКА ДЛЯ LINUX
# ============================================
	g++ main.cpp src/back/cJSON.c src/back/onnx_model.cpp src/back/bert_onnx_inference.cpp src/back/file_processor.cpp src/back/simple_tokenizer.cpp -s -O2 -lonnxruntime -pthread -o predictor.out

# ============================================
# ДИНАМИЧЕСКАЯ СБОРКА ДЛЯ WINDOWS (x64)
# ============================================

# Пути
PROJECT_ROOT := $(CURDIR)
SRC_DIR      := $(PROJECT_ROOT)/src
BUILD_DIR    := $(PROJECT_ROOT)/build-dynamic
EXE_NAME     := AdverbialDetector.exe
DIST_DIR     := $(PROJECT_ROOT)/dist

# Windows SDK и CRT
WIN_SDK_ROOT := $(HOME)/.xwin/sdk
CRT_ROOT     := $(HOME)/.xwin/crt

# ONNX Runtime
ONNX_DIR     := $(SRC_DIR)/onnxruntime-win-x64
ONNX_INC     := $(ONNX_DIR)/include
ONNX_LIB     := $(ONNX_DIR)/lib

# ===== КОМПИЛЯТОР =====
TARGET       := x86_64-windows-msvc
CXX          := clang++
CC           := clang
LD           := lld-link

# ===== ФЛАГИ КОМПИЛЯЦИИ (ДИНАМИЧЕСКИЕ) =====
# -MD = Динамическая CRT (MultiThreaded DLL)
CXXFLAGS     := -target $(TARGET) \
                --sysroot="$(WIN_SDK_ROOT)" \
                -I"$(SRC_DIR)" \
                -I"$(ONNX_INC)" \
                -I"$(CRT_ROOT)/include" \
                -I"$(WIN_SDK_ROOT)/include/ucrt" \
                -I"$(WIN_SDK_ROOT)/include/um" \
                -I"$(WIN_SDK_ROOT)/include/shared" \
                -std=c++17 \
                -O2 \
                -Wno-deprecated-declarations \
                -Wno-ignored-attributes \
                -Wno-nonportable-include-path \
                -Wno-pragma-pack \
                -Wno-ignored-pragma-intrinsic \
                -DUNICODE -D_UNICODE \
                -DWIN32 -D_WIN32 -D_WINDOWS \
                -DNOMINMAX \
                -D_CRT_SECURE_NO_WARNINGS \
                -D_HAS_EXCEPTIONS=1 \
                -D_DLL \
                -D_MT \
                -fexceptions \
                -fms-extensions \
                -fms-compatibility \
                -fdelayed-template-parsing \
                -m64 \
                -mtune=generic \
                -mstackrealign \
                -mstack-alignment=16 \
                -MD

CFLAGS       := -target $(TARGET) \
                --sysroot="$(WIN_SDK_ROOT)" \
                -I"$(SRC_DIR)" \
                -I"$(CRT_ROOT)/include" \
                -I"$(WIN_SDK_ROOT)/include/ucrt" \
                -std=c11 \
                -O2 \
                -m64 \
                -mstackrealign \
                -mstack-alignment=16 \
                -D_DLL \
                -D_MT \
                -MD \
                -D_CRT_SECURE_NO_WARNINGS

# ===== ФЛАГИ ЛИНКОВКИ =====
WIN_LIB_PATH := $(WIN_SDK_ROOT)/lib/um/x86_64
UCRT_LIB_PATH := $(WIN_SDK_ROOT)/lib/ucrt/x86_64
CRT_LIB_PATH := $(CRT_ROOT)/lib/x86_64

LDFLAGS      := -libpath:"$(ONNX_LIB)" \
                -libpath:"$(WIN_LIB_PATH)" \
                -libpath:"$(UCRT_LIB_PATH)" \
                -libpath:"$(CRT_LIB_PATH)" \
                -subsystem:console \
                -nologo \
                -entry:mainCRTStartup \
                -stack:0x2000000 \
                -dynamicbase \
                -nxcompat \
                -guard:cf

# ===== ДИНАМИЧЕСКИЕ БИБЛИОТЕКИ =====
LIBS         := onnxruntime.lib \
                kernel32.lib \
                user32.lib \
                advapi32.lib \
                ole32.lib \
                oleaut32.lib \
                ws2_32.lib \
                bcrypt.lib \
                mswsock.lib \
                msvcrt.lib \
                vcruntime.lib \
                ucrt.lib

# ===== ИСХОДНЫЕ ФАЙЛЫ =====
CPP_SOURCES  := main.cpp \
                $(SRC_DIR)/AdverbialDetector.cpp \
                $(SRC_DIR)/TextProcessor.cpp
C_SOURCES    := $(SRC_DIR)/cJSON.c

CPP_OBJS     := $(patsubst %.cpp,$(BUILD_DIR)/%.obj,$(notdir $(CPP_SOURCES)))
C_OBJS       := $(patsubst %.c,$(BUILD_DIR)/%.obj,$(notdir $(C_SOURCES)))
OBJS         := $(CPP_OBJS) $(C_OBJS)

# ===== ЦЕЛИ =====
.PHONY: all clean run verify check package

all: $(BUILD_DIR)/$(EXE_NAME)

# Основная цель
$(BUILD_DIR)/$(EXE_NAME): $(OBJS)
	@echo "🔗 Динамическая линковка..."
	@mkdir -p $(BUILD_DIR)
	$(LD) $(LDFLAGS) -out:$@ $^ $(LIBS)
	@echo "✅ Динамический исполняемый файл создан: $@"
	@echo "📦 Размер: $$(stat -c%s $@ 2>/dev/null || stat -f%z $@) байт"

# Компиляция main.cpp
$(BUILD_DIR)/main.obj: main.cpp
	@echo "⚙️  Компиляция main.cpp (динамически)..."
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Компиляция AdverbialDetector.cpp
$(BUILD_DIR)/AdverbialDetector.obj: $(SRC_DIR)/AdverbialDetector.cpp
	@echo "⚙️  Компиляция AdverbialDetector.cpp (динамически)..."
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Компиляция TextProcessor.cpp
$(BUILD_DIR)/TextProcessor.obj: $(SRC_DIR)/TextProcessor.cpp
	@echo "⚙️  Компиляция TextProcessor.cpp (динамически)..."
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Компиляция cJSON.c
$(BUILD_DIR)/cJSON.obj: $(SRC_DIR)/cJSON.c
	@echo "⚙️  Компиляция cJSON.c (динамически)..."
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Проверка динамических зависимостей
verify: $(BUILD_DIR)/$(EXE_NAME)
	@echo "🔍 Проверка динамических зависимостей..."
	@echo "Необходимые DLL:"
	@objdump -p $(BUILD_DIR)/$(EXE_NAME) 2>/dev/null | grep -i "dll name" | sort | uniq || echo "Не удалось прочитать зависимости"
	@echo ""
	@echo "📊 Список импортов:"
	@objdump -p $(BUILD_DIR)/$(EXE_NAME) 2>/dev/null | grep -A10 "Import Address Table" || true

# Проверка флагов компиляции
check:
	@echo "🧪 Проверка флагов компиляции..."
	@echo "CXXFLAGS: $(CXXFLAGS)" | tr ' ' '\n' | head -20
	@echo ""
	@echo "Проверка макросов:"
	@$(CXX) $(CXXFLAGS) -E -dM - < /dev/null 2>&1 | grep -E "_MT|_DLL|_STATIC|_DEBUG" | sort

# Создание дистрибутива с DLL
package: $(BUILD_DIR)/$(EXE_NAME)
	@echo "📦 Создание дистрибутива..."
	@mkdir -p $(DIST_DIR)
	
	# Копируем EXE
	cp $(BUILD_DIR)/$(EXE_NAME) $(DIST_DIR)/
	@echo "✅ $(EXE_NAME)"
	
	# Копируем ONNX Runtime DLL
	if [ -f "$(ONNX_LIB)/onnxruntime.dll" ]; then \
		cp "$(ONNX_LIB)/onnxruntime.dll" $(DIST_DIR)/; \
		echo "✅ onnxruntime.dll"; \
	else \
		echo "⚠️  onnxruntime.dll не найден в $(ONNX_LIB)"; \
		echo "💡 Скачайте из: https://github.com/microsoft/onnxruntime/releases"; \
	fi
	
	# Список требуемых DLL (только информация)
	@echo ""
	@echo "📋 На Windows также потребуются:"
	@echo "   • vcruntime140.dll (Visual C++ Runtime)"
	@echo "   • msvcp140.dll (Microsoft C++ Runtime)"
	@echo "   • ucrtbase.dll (Universal CRT)"
	@echo ""
	@echo "📁 Дистрибутив создан в: $(DIST_DIR)"
	@ls -la $(DIST_DIR)/ 2>/dev/null || echo "Содержимое $(DIST_DIR):" && ls -l $(DIST_DIR)/

# Запуск через Wine
run: $(BUILD_DIR)/$(EXE_NAME)
	@echo "🚀 Запуск через Wine..."
	@echo "================================"
	# Добавляем путь к ONNX DLL для Wine
	if [ -f "$(ONNX_LIB)/onnxruntime.dll" ]; then \
		WINEDLLPATH="$(ONNX_LIB):$$WINEDLLPATH" wine $(BUILD_DIR)/$(EXE_NAME) || echo "⚠️  Код возврата: $$?"; \
	else \
		wine $(BUILD_DIR)/$(EXE_NAME) || echo "⚠️  Код возврата: $$?"; \
	fi

# Очистка
clean:
	@echo "🧹 Очистка динамической сборки..."
	rm -rf $(BUILD_DIR) $(DIST_DIR)
	@echo "✅ Очищено"

# Простая сборка (все в одной команде)
simple:
	@echo "🚀 Простая динамическая сборка..."
	@mkdir -p $(BUILD_DIR)
	
	# 1. Компиляция
	@echo "1. Компиляция..."
	$(CXX) $(CXXFLAGS) -c main.cpp -o $(BUILD_DIR)/main.obj
	$(CXX) $(CXXFLAGS) -c $(SRC_DIR)/AdverbialDetector.cpp -o $(BUILD_DIR)/AdverbialDetector.obj
	$(CXX) $(CXXFLAGS) -c $(SRC_DIR)/TextProcessor.cpp -o $(BUILD_DIR)/TextProcessor.obj
	$(CC) $(CFLAGS) -c $(SRC_DIR)/cJSON.c -o $(BUILD_DIR)/cJSON.obj
	
	# 2. Линковка
	@echo "2. Динамическая линковка..."
	cd $(BUILD_DIR) && \
	$(LD) \
	  -libpath:"$(ONNX_LIB)" \
	  -libpath:"$(WIN_LIB_PATH)" \
	  -libpath:"$(UCRT_LIB_PATH)" \
	  -libpath:"$(CRT_LIB_PATH)" \
	  -subsystem:console \
	  -nologo \
	  -entry:mainCRTStartup \
	  -stack:0x2000000 \
	  -out:$(EXE_NAME) \
	  *.obj \
	  $(LIBS)
	
	@echo "✅ Готово: $(BUILD_DIR)/$(EXE_NAME)"
	@ls -lh $(BUILD_DIR)/$(EXE_NAME) 2>/dev/null || ls -l $(BUILD_DIR)/$(EXE_NAME)
	@echo ""
	@echo "⚠️  Внимание: требуется onnxruntime.dll и MSVC Runtime"

.PHONY: all clean run verify check package simple
