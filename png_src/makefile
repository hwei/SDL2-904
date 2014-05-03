TARGET_DIR:=../SDL2-904/Resources
BUILD_DIR:=./build_res
PNGS:=$(shell find . -name '*.png')
WEBPS:=$(addprefix $(TARGET_DIR)/,$(PNGS:%.png=%.webp))
LIST:=res.lst

all: $(WEBPS)

$(LIST): $(WEBPS)
	cd 

$(TARGET_DIR)/%.webp: %.png
	cwebp -lossless $< -o $@
