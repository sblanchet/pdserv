
BUILD_DIR:=build
INST_DIR:=/opt/etherlab


default: all

help:
	@echo "Build pdserv"
	@echo "Syntax make [COMMAND]"
	@echo "COMMAND:"
	@echo "    all         build all binaries"
	@echo "    clean       clean building object"
	@echo "    help        show this help"
	@echo "    install     install binaries"


all:
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && \
	cmake -DCMAKE_INSTALL_PREFIX:PATH=$(INST_DIR) \
	      -DCMAKE_INSTALL_LIBDIR=lib \
	      --build ..
	make -C $(BUILD_DIR)


clean:
	rm -fr $(BUILD_DIR)

install: all
	make -C $(BUILD_DIR) install


.PHONY: all clean default help install
