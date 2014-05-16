PACK:=../SDL2-904/Resources/res.pack

BUILD_DIR:=./build
PNG_DIR:=../png_src
SHADER_DIR:=../shader_src
PACK_DIR:=$(dir $(PACK))
WEBP_DIR:=$(BUILD_DIR)/webp
DIRS:=$(PACK_DIR) $(WEBP_DIR)

PNGS:=$(shell find "$(PNG_DIR)" -name '*.png')
WEBPS:=$(patsubst $(PNG_DIR)/%.png,$(WEBP_DIR)/%.webp,$(PNGS))
SHADERS:=$(shell find "$(SHADER_DIR)" -type f)
WEBP_LIST:=$(BUILD_DIR)/webp.lst
SHADER_LIST:=$(BUILD_DIR)/shader.lst
FULL_LIST:=$(BUILD_DIR)/full.lst

all: $(PACK)

clean:
	rm -f $(PACK)
	rm -f $(WEBP_LIST)
	rm -rf $(WEBP_DIR)

$(PACK): $(FULL_LIST) | $(PACK_DIR)
	./pack.py -o $@ -l $<

$(FULL_LIST): $(WEBP_LIST) $(SHADER_LIST)
	cat $^ > $@

$(WEBP_LIST): $(WEBPS)
	./make_res_list "$(WEBP_DIR)" > $@

$(SHADER_LIST): $(SHADERS)
	./make_res_list "$(SHADER_DIR)" > $@

$(WEBP_DIR)/%.webp: $(PNG_DIR)/%.png | $(WEBP_DIR)
	cwebp -lossless $< -o $@

$(DIRS):
	mkdir -p $@
