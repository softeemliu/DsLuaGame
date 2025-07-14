local next = next
local tInsert, tRemove = table.insert, table.remove

function typeCompare(prototype, nowType)
	if nowType == false then
		return true
	end
	local pType = type(prototype)
	local nType = type(nowType)
	if pType ~= nType then
		assert("type not the same")
		return false
	end
	if "table" == pType then
		if prototype.className ~= nowType.__className then
			assert(nil, "class type not the same" .. nowType.__className .. ":" .. prototype.className)
			return false
		end
	end
	return true
end

-- 继承一个class类, base为基类，className
function inheritClass(base,className)
	local className = setmetatable({}, { __index = base })
	className.__index = className
	function className:new()
		local obj = base.new(self) 
		setmetatable(obj,self)
		return obj
	end
	return className
end

-- 创建一个class(无继承关系)
function class(className)
	local className = {}
	className.__index = className
	className.__name = className
	function className:new()
		local obj = setmetatable({}, className)
		return obj
	end
	return className
end

local arrayBaseFunc = {}
function arrayBaseFunc:pushBack(value)
	if not typeCompare(rawget(self, "valuePrototype"), value) then
		assert(nil, "Array pushBack Error, type is not match")
		return false
	end
	tInsert(rawget(self, "__array__"), value)
	return true
end

function arrayBaseFunc:getSize()
	return #rawget(self, "__array__")
end

function arrayBaseFunc:insert(pos, value)
	if not typeCompare(rawget(self, "valuePrototype"), value) then
		assert(nil, "input Error, type is not match,array type:")
		return false
	end
	tInsert(rawget(self, "__array__"), pos, value)
	return true
end

function arrayBaseFunc:remove(pos)
	return tRemove(rawget(self, "__array__"), pos)
end

function arrayBaseFunc:getArray()
	return rawget(self, "__array__")
	
end

function arrayBaseFunc:clear()
	rawset(self, "__array__", {})
end

function arrayBaseFunc:__getValueBase()
	return rawget(self, "_valueBase")
end

function arrayBaseFunc:iter()
	local a = rawget(self, "__array__")
	return  function (a, k)
		local rk, rv 
		if k then 	
			rk, rv = next(a, k)
		else 
			rk, rv = next(a)
		end 
		return rk, rv 
	end, a
end

function arrayBaseFunc:__write(__os)
	writeArray(__os, self)
end

function arrayBaseFunc:__read(__is)
	readArray(__is, self)
end

local ArrayMetatable = {
__index = function(t, k)
			if type(k) == "number" then
				local a = rawget(t, "__array__")
				local size = #a
				if k > size then
					assert(nil, "Array bounds read,not size:" .. size .. ",now indexing:" .. k)
					return false
				end
				return a[k]
			else
				local func = arrayBaseFunc[k]
				if func then
					return func
				else
					return nil
				end
			end
		end,
__newindex = function (t, k, v)
			local a = rawget(t, "__array__")
			local size = #a
			if k > size then
				assert(nil,"Array bounds read,not size:" .. size .. ",now indexing:" .. k)
				return false
			end
			if not typeCompare(rawget(t, "valuePrototype"), v) then
				assert(nil, "input Error, type is not match,array type:")
				return false
			end
			a[k] = v
		end,
}

function createArray(value)
	local Array = {
		__value__ = value
	}
	local valueBase = nil
	if value["__cdlbase__"] and value.__cdlbase__ == true then
		valueBase = value
		Array.__value__ = valueBase.init
	end
	function Array.new()
	    local ArrayBase = {
			__array__ = {},
			valuePrototype = Array.__value__,
			_valueBase = valueBase,
		}
		setmetatable(ArrayBase, ArrayMetatable)
		return ArrayBase
	end
	return Array
end

function readArray(is, array)
	local size = is:getIntSize()
	local valuePrototype = rawget(array, "valuePrototype")
	local _valueBase = rawget(array, "_valueBase")
	if _valueBase then
		array:clear()
        if size > 0 then
            is:setUseBitMark(false)
        end
		for i = 1, size do
			local arrayValue = _valueBase.read(is)
			array:pushBack(arrayValue)
		end
		if size > 0 then
            is:setUseBitMark(true)
        end
	else
		local _oldSize = array:getSize()
		local _tempArray = array.__array__
		array:clear()

		local arrayValue
		for i = 1, size do
			if i <= _oldSize then
				arrayValue = _tempArray[i]
			else
				arrayValue = valuePrototype.new()
			end
			arrayValue:__read(is)
			array:pushBack(arrayValue)
		end
	end
end

function writeArray(os, array)
	os:writeIntSize(array:getSize())
	local valuePrototype = rawget(array, "valuePrototype")
	local _valueBase = rawget(array, "_valueBase")
	if _valueBase then
        if array:getSize() > 0 then
            os:setUseBitMark(false)
        end
		for k, v in ipairs(array:getArray()) do
			_valueBase.write(os, v)
		end
		if array:getSize() > 0 then
            os:setUseBitMark(true)
        end
	else
		for k, v in ipairs(array:getArray()) do
			v:__write(os)
		end
	end
end

local DateMetatable = { 
__newindex = function(t, k)
					assert(false, "attemp to newindex value " .. t)
					print("attemp to newindex value", t)
				end,
__index = function(t, k)
			assert(false, "attemp to index not exist value " .. k)
			print("attemp to index not exist value", k)
		end
}

function createNormalEnum(enumTableInput)
	return createEnum(enumTableInput, true)
end

local EnumMetatable = { 
__newindex = function(t, k)
					assert(false, "attemp to newindex value " .. t)
					print("attemp to newindex value", t)
				end,
__index = function(t, k)
			assert(false, "attemp to index not exist value " .. k)
			print("attemp to index not exist value", k)
		end
}

function createEnum(enumTableInput, normal)
	local enumTable = {}
	enumTable.__enumSet__ = {}
	local maxEnum = 0
	for k,v in pairs(enumTableInput) do
		enumTable.__enumSet__[v] = true
		enumTable[k] = v
		if maxEnum < v then
			maxEnum = v
		end
	end
	
	if not normal then
		if maxEnum <= 0x7f then
			function enumTable:__write(__os, enum)
				if false == self:isEnum(enum) then
					error("enumWriteError vale of " .. enum)
				end
				__os:writeByte(enum)
			end
			function enumTable:__read(__is)
				local enum = __is:readByte()
				if false == self:isEnum(enum) then
					error("enumReadError vale of " .. enum)
				end
				return enum
			end
		elseif maxEnum <= 0x7fff then
			function enumTable:__write(__os, enum)
				if false == self:isEnum(enum) then
					error("enumWriteError vale of " .. enum)
				end
				__os:writeShort(enum)
			end
			function enumTable:__read(__is)
				local enum = __is:readShort()
				if false == self:isEnum(enum) then
					error("enumReadError vale of " .. enum)
				end
				return enum
			end
		else
			function enumTable:__write(__os, enum)
				if false == self:isEnum(enum) then
					error("enumWriteError vale of " .. enum)
				end
				__os:writeInt(enum)
			end
			function enumTable:__read(__is)
				local enum = __is:readInt()
				if false == self:isEnum(enum) then
					error("enumReadError vale of " .. enum)
				end
				return enum
			end
		end
	end
	function enumTable:isEnum(v)
		if enumTable.__enumSet__[v] then
			return true
		end
		return false
	end
	
	setmetatable(enumTable, EnumMetatable)

	return enumTable
end

Pair = class("Pair")
function Pair:init(param)
	if param then
		self.key = param.key or false
		self.value = param.value or false
	else
		self.key = false
		self.value = false
	end
end

function MakePair(key, value)
	return Pair.new({key = key, value = value })
end

function makeKey(key)
	local keyStr = ""
	local t = type(key)

	if "number" == t then
		return keyStr .. key
	elseif "string" == t then	
		return "s" .. key
	end	
	if "table" == t then
		for k,v in pairs(key) do
			local tvalue = type(v)
			if "function" ~= tvalue and "table" ~= tvalue then
				keyStr = keyStr .. v .. ","
			end
		end
		return keyStr
	end
	if "function" == t then
		assert("function" == t)
		print("key can not be function")
		return nil
	end
	assert(nil,"keyStr Error key type:" .. type(key) .. ",key:" .. key)
	return nil
end

DictionaryEnd = 0
local dictionBaseFunc = {}
function dictionBaseFunc:find(key)
	return self[key]
end

function dictionBaseFunc:getDictionary()
	return rawget(self, "__pairs__")
end

function dictionBaseFunc:insert(pair , force)
	if pair.__className ~= "Pair" then
		assert(nil, "not a pair value")
		return
	end

	if not (typeCompare(rawget(self, "keyPrototype"), pair.key) and typeCompare(rawget(self, "valuePrototype"),pair.value) ) then
		assert(nil, "type error")
		return
	end

	local keyStr = makeKey(pair.key)

	local d = rawget(self, "__pairs__")

	if d[keyStr] then
		if force and force == true then
			d[keyStr] = pair
		end
		return false
	end

	d[keyStr] = pair

	self.length = self.length + 1

	return true
end

function dictionBaseFunc:erase(key)
	if not typeCompare(self.keyPrototype, key) then
		return false
	end

	local keyStr = makeKey(key)

	local d = rawget(self, "__pairs__")

	if d[keyStr] then
		d[keyStr] = nil
		self.length = self.length - 1
		assert(self.length >= 0, "length error")
	end
end

function dictionBaseFunc:clear()
	rawset(self, "__pairs__", {})
	self.length = 0
end


function dictionBaseFunc:__quickInsert(pair)
	local keyStr = makeKey(pair.key)
	self.__pairs__[keyStr] = pair
	self.length = self.length + 1
end



function dictionBaseFunc:size()
	return self.length
end

function dictionBaseFunc:iter()
	local d = self.__pairs__
	return  function (d, k)
		local rk, rv 
		if k then 	
			rk, rv = next(d, k)
		else 
			rk, rv = next(d)
		end 
		local v
		if rv then
			v = rv.value
		end
		return rk, v 
	end, d
end

function dictionBaseFunc:__write(__os)
	writeDictionary(__os, self)
end

function dictionBaseFunc:__read(__is)
	readDictionary(__is, self)
end
	
local dictionaryMetatable = {
__index = function(t, k)
		if type(k) == "string" then
			local func = dictionBaseFunc[k]
			if func then
				return func
			end
		end
		if typeCompare(rawget(t, "keyPrototype"), k) then
			local pair = t.__pairs__[ makeKey(k) ]
			if pair then
				return pair.value
			else
				return nil
			end
		end												
	end,
__newindex = function (t, k, v)
		local pair = Pair.new()
		pair.key = k
		pair.value = v
		t:insert(pair, true)
		if rawget(t, k) then
			assert(nil, "string key has in dictionBaseFunc, which is" .. k)
		end
	end,
}

function createDictionary(key, value)
	local Dictionary = { 
		__key__ = key,
		__value__ = value,
	}
	local keyBase = nil
	if key["__cdlbase__"] ~= nil and key.__cdlbase__ == true then
		keyBase = key
		Dictionary.__key__ = keyBase.init
	end
	
	local valueBase = nil
	if value["__cdlbase__"] ~= nil and value.__cdlbase__ == true then
		valueBase = value
		Dictionary.__value__ = valueBase.init
	end
	
	function Dictionary.new()
		local dict = {
			__pairs__ = {},
			keyPrototype = Dictionary.__key__,
			_keyBase = keyBase,
			valuePrototype = Dictionary.__value__,
			_valueBase = valueBase,
			length = 0,
		}
		setmetatable(dict, dictionaryMetatable)
		return dict
	end

	return Dictionary
end

function readDictionary(is, dictionary)
	local size = is:readSize()
	local keyPrototype = rawget(dictionary, "keyPrototype")
	local _keyBase = rawget(dictionary, "_keyBase")
	local valuePrototype = rawget(dictionary, "valuePrototype")
	local _valueBase = rawget(dictionary, "_valueBase")
	
	local _tempPairs = dictionary.__pairs__
	dictionary:clear()

	local tempKey
	local oldValue
	for k = 1, size , 1 do
		local p = Pair.new()
		if _keyBase then
			p.key = _keyBase.read(is)
		else
			p.key = keyPrototype.new()
			p.key:__read(is)
		end

		if _valueBase then
			p.value = _valueBase.read(is)
		else
			tempKey, oldValue = next(_tempPairs)
			if tempKey then
				p.value = oldValue.value
				_tempPairs[tempKey] = nil
			else
				p.value = valuePrototype.new()
			end
			p.value:__read(is)
		end
		dictionary:__quickInsert(p)
	end
end

function writeDictionary(os, dictionary)
	os:writeSize(dictionary:size())
	local keyPrototype = rawget(dictionary, "keyPrototype")
	local _keyBase = rawget(dictionary, "_keyBase")
	local valuePrototype = rawget(dictionary, "valuePrototype")
	local _valueBase = rawget(dictionary, "_valueBase")
	for k,v in pairs(rawget(dictionary, "__pairs__")) do
		if _keyBase then
			_keyBase.write(os, v.key)
		else
			v.key:__write(os)
		end
		if _valueBase then
			_valueBase.write(os, v.value)
		else
			v.value:__write(os)
		end
	end
end