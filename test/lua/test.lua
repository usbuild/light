light = require("light")
print("hello")

function on_install(fd)
	print("fd2: "..fd)
	msg = light.ConnectRequestBuilder.new()
	light.post(0, 1, msg)
end

function on_message(sType, from, data)

print("sType "..sType.." from "..from)
end