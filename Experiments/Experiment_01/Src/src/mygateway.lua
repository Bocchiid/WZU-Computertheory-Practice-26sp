local fs = require "nixio.fs"

m = Map("mygateway", translate("物联网关"), translate("修改mygateway软件包的配置。"))

s = m:section(TypedSection, "mygateway", "")
s.addremove = false
s.anonymous = true

view_period = s:option(Value, "period", translate("发送间隔"))
view_period.default = 0

view_output = s:option(TextValue, "output", "消息内容")
view_output.rmempty = false
view_output.rows = 2

m.on_after_commit = function(self)
	luci.sys.call("/etc/init.d/mygateway restart > /dev/null")
end

return m
