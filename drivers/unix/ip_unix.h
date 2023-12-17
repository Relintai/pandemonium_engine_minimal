#ifndef IP_UNIX_H
#define IP_UNIX_H

/*  ip_unix.h                                                            */


#include "core/io/ip.h"

#if defined(UNIX_ENABLED) || defined(WINDOWS_ENABLED)

class IP_Unix : public IP {
	GDCLASS(IP_Unix, IP);

	virtual void _resolve_hostname(List<IP_Address> &r_addresses, const String &p_hostname, Type p_type = TYPE_ANY) const;

	static IP *_create_unix();

public:
	virtual void get_local_interfaces(RBMap<String, Interface_Info> *r_interfaces) const;

	static void make_default();
	IP_Unix();
};

#endif
#endif // IP_UNIX_H
