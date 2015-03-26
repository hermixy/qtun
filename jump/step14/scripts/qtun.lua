qtun = {
	log = {
		syslog = function (level, fmt, ...)
			_syslog(level, string.format(fmt, ...))
		end
	}
}
