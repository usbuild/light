light = require("light")
print("hello")

function on_install(fd)
	print("fd2: "..fd)
	light.post(0, 1, light.create_message())
end

function on_message(sType, from, data)

print("sType "..sType)
end