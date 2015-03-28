qtun = {
	log = {
		syslog = function (level, fmt, ...)
			_syslog(level, string.format(fmt, ...))
		end
	}
}

function trim(s)
	return s:gsub('^%s*(.-)%s*$', '%1')
end

function is_numeric(s)
	return tonumber(s) ~= nil
end

function is_string(s)
	return not is_numeric(s)
end