-- bidir test script

-- these variables will be accessed from C++, they must be global
nWidth = 1024
nHeight = 768
sPath = "../../bin/lua"

-- Modify variable set in C++
-- gfFresnel = 6.7
-- gsName = "Liew"
gnDrag = 1000

function Application:uselessMethod()
	print("Yes I am useless, testInt: "..self.testInt);
end

-- this function will be called from C++
function multiCall(...)
	print("multiCall lua function start")

	-- test c++ variables mod
	print("From C++ to Lua Width["..nWidth.."] Height["..nHeight.."] Path["..sPath.."]")
	
	-- test lua call
	gnDrag = 5
	print("Lua access C++ variable Drag["..gnDrag.."] Fresnel["..gfFresnel.."] Name["..gsName.."]")

	-- test c++ function called
	local tot, avg = Average(6, 1, 2, 3, 4, 5, 6)
	print("From lua call C++ Average: Total["..tot.."] Average["..avg.."]")
	
	local App = Application.Create()
	local App = Application.Create()
	print("In Lua, Application testInt:"..App.testInt.." testFloat: "..App.testFloat.." testStr: "..App.testStr)
	App.testInt = 34
	App.testFloat = 34.34
	App.testStr = "YesItWorks"
	
	local Pg = Page.Create("ScriptPage", 5, 5, 320, 240, 3)
	print("In Lua, Page X:"..Pg.x.." id: "..Pg.id.." title: "..Pg.title)

	if App:SetContent(1) then
		print("SetContent(1) pass")
	else
		print("SetContent(1) failed, try SetContent(0)")
		App:SetContent(0)
	end
		
	return ...
end
