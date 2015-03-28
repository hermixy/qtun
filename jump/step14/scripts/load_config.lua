local function append_key_value(key, value)
	if value == 'true' then
		value = true
	elseif value == 'false' then
		value = false
	elseif is_numeric(value) then
		value = tonumber(value)
	elseif key == 'log_level' then
		if is_string(value) then
			value = qtun.log[value]
		end
	end
	qtun.conf[key] = value
end

local function set_true(key)
	qtun.conf[key] = true
end

local file = io.open('config', 'r')
while true do
::start::
	local line = file:read('*line')
	if line == nil then break end
	
	local len = #line
	local key = ''
	local first = ''
	local start = 0
	local have_key = false
	local have_value = false
	local added = false
	for i=0, len do
		local ch = line:sub(i + 1, i + 1)
		if first == '' and ch ~= ' ' then
			first = ch
		end
		if ch == '#' then
			if trim(line:sub(start, i)) == '' then goto start end
			have_value = true
			if have_key then
				append_key_value(key, trim(line:sub(start, i)))
			else
				set_true(trim(line:sub(start, i)))
			end
			added = true
			break
		elseif ch == '=' then
			key = trim(line:sub(start, i))
			if key == '' then
				error('missing key')
			end
			have_key = true
			start = i + 2
		end
	end
	if first == '#' then goto start end
	
	if not added then
		if not have_key then
			set_true(trim(line:sub(start, i)))
		else
			append_key_value(key, trim(line:sub(start, i)))
		end
	end
end
file:close()