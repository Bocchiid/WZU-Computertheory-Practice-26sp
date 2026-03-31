module("luci.controller.other",package.seeall)

function index()
	entry({"admin", "other"}, {}, _("其他"),60).index = true
	if nixio.fs.access("/etc/config/mygateway") then
		entry({"admin", "other", "mygateway"}, cbi("mygateway"), _("物联网关"), 1)
	end
end
