include $(TOPDIR)/rules.mk

PKG_NAME:=mygateway
PKG_VERSION:=1.0
PKG_RELEASE:=1
SOURCE_DIR:=$(shell pwd)/src
include $(INCLUDE_DIR)/package.mk

define Package/mygateway
	SECTION:=examples
	CATEGORY:=Examples
	TITLE:=An IoT Gateway!
	DEPENDS:=+libuci +libpthread +libmosquitto-nossl
endef

define Package/mygateway/description
	This is an IoT gateway project.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	cp $(SOURCE_DIR)/* $(PKG_BUILD_DIR)
	$(Build/Patch)
endef

define Build/Compile
	$(TARGET_CC) $(TARGET_CFLAGS) -o $(PKG_BUILD_DIR)/mygateway.o -c $(PKG_BUILD_DIR)/mygateway.c
	$(TARGET_CC) $(TARGET_CFLAGS) -o $(PKG_BUILD_DIR)/config.o -c $(PKG_BUILD_DIR)/config.c
	$(TARGET_CC) $(TARGET_CFLAGS) -o $(PKG_BUILD_DIR)/comport.o -c $(PKG_BUILD_DIR)/comport.c
	$(TARGET_CC) $(TARGET_CFLAGS) -o $(PKG_BUILD_DIR)/comport_receiver.o -c $(PKG_BUILD_DIR)/comport_receiver.c
	$(TARGET_CC) $(TARGET_CFLAGS) -o $(PKG_BUILD_DIR)/mqtt_client.o -c $(PKG_BUILD_DIR)/mqtt_client.c
	$(TARGET_CC) $(TARGET_LDFLAGS) -o $(PKG_BUILD_DIR)/$1 $(PKG_BUILD_DIR)/mygateway.o $(PKG_BUILD_DIR)/config.o \
	$(PKG_BUILD_DIR)/comport.o $(PKG_BUILD_DIR)/comport_receiver.o $(PKG_BUILD_DIR)/mqtt_client.o -luci -lpthread -lmosquitto
endef

define Package/mygateway/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/mygateway $(1)/usr/bin
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) ./src/mygateway.config $(1)/etc/config/mygateway
	$(INSTALL_DIR) $(1)/usr/lib/lua/luci/controller
	$(INSTALL_CONF) ./src/other.lua $(1)/usr/lib/lua/luci/controller/other.lua
	$(INSTALL_DIR) $(1)/usr/lib/lua/luci/model/cbi
	$(INSTALL_CONF) ./src/mygateway.lua $(1)/usr/lib/lua/luci/model/cbi/mygateway.lua
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./src/mygateway $(1)/etc/init.d/mygateway
endef

$(eval $(call BuildPackage,mygateway))
