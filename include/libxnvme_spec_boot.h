/**
 * What the boot specification defines, by xnvme_spec
 *
 * Written from the data model of senfd 0.19.0, by toolbox/xnvme_spec_headers.py;
 * each name and mask is as the figure it cites gives it. Do not
 * edit: see the contributor docs on producing the spec headers.
 *
 * @file libxnvme_spec_boot.h
 */
#ifndef LIBXNVME_SPEC_BOOT_H
#define LIBXNVME_SPEC_BOOT_H

#include <stdint.h>

/**
 * System UUID Value Table (Boot 1.4, 3.1.1, Figure 5, page 16, PDF page 22)
 */
struct __attribute__((packed)) xnvme_spec_boot_system_uuid_value_table {
	uint32_t time_low; ///< Time Low, bytes 3:0
	uint16_t time_mid; ///< Time Mid, bytes 5:4
	uint16_t time_hi_and_versionb; ///< Time Hi and versionb, bytes 7:6
	uint8_t clock_seq_hi_and_rsvd; ///< clock_seq_hi_and_rsvd, byte 8
	uint8_t clock_seq_l; ///< clock_seq_l, byte 9
	uint8_t node[6]; ///< Node, bytes 15:10
};

/**
 * NBFT Table (Boot 1.4, 3.2.2.1, Figure 9, page 20, PDF page 26)
 */
struct __attribute__((packed)) xnvme_spec_boot_nbft_table {
	uint8_t sig[4]; ///< Signature, bytes 3:0
	uint32_t len; ///< Length, bytes 7:4
	uint8_t majrev; ///< Major Revision, byte 8
	uint8_t cksum; ///< Checksum, byte 9
	uint8_t oemid[6]; ///< OEM ID, bytes 15:10
	uint64_t oemtid; ///< OEM Table ID, bytes 23:16
	uint32_t oemrev; ///< OEM Revision, bytes 27:24
	uint32_t crid; ///< Creator ID, bytes 31:28
	uint32_t crrev; ///< Creator Revision, bytes 35:32
	uint32_t ho; ///< Heap Offset, bytes 39:36
	uint32_t hl; ///< Heap Length, bytes 43:40
	uint8_t dshor[6]; ///< Driver Signature Heap Object Reference, bytes 49:44
	uint8_t minrev; ///< Minor Revision, byte 50
	uint8_t rsvd51[13]; ///< Reserved, bytes 63:51
	uint8_t cdstructid; ///< Structure ID, byte 64
	uint8_t cdmajrev; ///< Major Revision, byte 65
	uint8_t cdminrev; ///< Minor Revision, byte 66
	uint8_t rsvd67; ///< Reserved, byte 67
	uint16_t cdcsl; ///< Control Structure Length, bytes 69:68
	uint8_t cdflags; ///< Flags, byte 70
	uint8_t rsvd71; ///< Reserved, byte 71
	uint8_t cdhdesc[6]; ///< Host Descriptor, bytes 77:72
	uint8_t cdhsv; ///< Host Descriptor Version, byte 78
	uint8_t rsvd79; ///< Reserved, byte 79
	uint32_t hfio; ///< HFI Descriptor List Offset, bytes 83:80
	uint16_t hfil; ///< HFI Descriptor Length, bytes 85:84
	uint8_t hfiv; ///< HFI Descriptor Version, byte 86
	uint8_t numhfi; ///< Number of Host Fabric Interface Descriptors, byte 87
	uint32_t ssnso; ///< SSNS Descriptor List Offset, bytes 91:88
	uint16_t ssnsl; ///< SSNS Descriptor Length, bytes 93:92
	uint8_t ssnsv; ///< SSNS Descriptor Version, byte 94
	uint8_t numssns; ///< Number of Subsystem Namespace Descriptors, byte 95
	uint32_t seco; ///< Security Profile Descriptor List Offset, bytes 99:96
	uint16_t secl; ///< Security Profile Descriptor Length, bytes 101:100
	uint8_t secv; ///< Security Profile Descriptor Version, byte 102
	uint8_t numsec; ///< Number of Security Profile Descriptors, byte 103
	uint32_t disco; ///< Discovery Descriptor Offset, bytes 107:104
	uint16_t discl; ///< Discovery Descriptor Length, bytes 109:108
	uint8_t discv; ///< Discovery Descriptor Version, byte 110
	uint8_t numdisc; ///< Number of Discovery Descriptors, byte 111
	uint8_t rsvd112[16]; ///< Reserved, bytes 127:112
};

/**
 * Host Descriptor (Boot 1.4, 3.2.2.3, Figure 10, page 23, PDF page 29)
 */
struct __attribute__((packed)) xnvme_spec_boot_host_descriptor {
	uint8_t structid; ///< Structure ID, byte 0
	uint8_t hflags; ///< Host Flags, byte 1, see XNVME_SPEC_BOOT_HOST_DESCRIPTOR_HFLAGS_*
	uint8_t hostid[16]; ///< Host ID, bytes 17:2
	uint8_t hnqnhor[6]; ///< Host NQN Heap Object Reference, bytes 23:18
	uint64_t rsvd24; ///< Reserved, bytes 31:24
};

/** Primary Administrative Host Descriptor (Boot 1.4, 3.2.2.3, Figure 10, page 23, PDF page 29); bits 4:3 of 'hflags' */
#define XNVME_SPEC_BOOT_HOST_DESCRIPTOR_HFLAGS_PAHD 0x18
/** Host NQN Configured (Boot 1.4, 3.2.2.3, Figure 10, page 23, PDF page 29); bit 2 of 'hflags' */
#define XNVME_SPEC_BOOT_HOST_DESCRIPTOR_HFLAGS_HNQNCON 0x4
/** HostID Configured (Boot 1.4, 3.2.2.3, Figure 10, page 23, PDF page 29); bit 1 of 'hflags' */
#define XNVME_SPEC_BOOT_HOST_DESCRIPTOR_HFLAGS_HIDCON 0x2
/** Descriptor Valid (Boot 1.4, 3.2.2.3, Figure 10, page 23, PDF page 29); bit 0 of 'hflags' */
#define XNVME_SPEC_BOOT_HOST_DESCRIPTOR_HFLAGS_DESCV 0x1

/**
 * The values of Primary Administrative Host Descriptor (Boot 1.4, 3.2.2.3, Figure 10, page 23, PDF page 29)
 */
enum xnvme_spec_boot_host_descriptor_hflags_pahd {
	XNVME_SPEC_BOOT_HOST_DESCRIPTOR_HFLAGS_PAHD_NOT_INDICATED_BY_DRIVER = 0x0, ///< Not Indicated by Driver (Boot 1.4, 3.2.2.3, Figure 10, page 23, PDF page 29)
	XNVME_SPEC_BOOT_HOST_DESCRIPTOR_HFLAGS_PAHD_UNSELECTED = 0x1, ///< Unselected (Boot 1.4, 3.2.2.3, Figure 10, page 23, PDF page 29)
	XNVME_SPEC_BOOT_HOST_DESCRIPTOR_HFLAGS_PAHD_SELECTED = 0x2, ///< Selected (Boot 1.4, 3.2.2.3, Figure 10, page 23, PDF page 29)
};

/**
 * Host Fabric Interface (HFI) Descriptor List (Boot 1.4, 3.2.2.4, Figure 11, page 25, PDF page 31)
 *
 * Followed, from byte 65, by as many Host Fabric Interface Descriptor n as the data says
 */
struct __attribute__((packed)) xnvme_spec_boot_host_fabric_interface_hfi_descriptor_list {
	uint8_t host_fabric_interface_descriptor_1[32]; ///< Host Fabric Interface Descriptor 1, bytes 31:0
	uint8_t host_fabric_interface_descriptor_2[33]; ///< Host Fabric Interface Descriptor 2, bytes 64:32
};

/**
 * Host Fabric Interface (HFI) Descriptor (Boot 1.4, 3.2.2.4, Figure 12, page 25, PDF page 31)
 */
struct __attribute__((packed)) xnvme_spec_boot_host_fabric_interface_hfi_descriptor {
	uint8_t structid; ///< Structure ID, byte 0
	uint8_t hfididx; ///< HFI Descriptor Index, byte 1
	uint8_t hfidflags; ///< HFI Descriptor Flags, byte 2, see XNVME_SPEC_BOOT_HOST_FABRIC_INTERFACE_HFI_DESCRIPTOR_HFIDFLAGS_*
	uint8_t hfitt; ///< HFI Transport Type, byte 3
	uint8_t rsvd4[12]; ///< Reserved, bytes 15:4
	uint8_t hfitidhor[6]; ///< HFI Transport Info Descriptor Heap Object Reference, bytes 21:16
	uint8_t rsvd22[10]; ///< Reserved, bytes 31:22
};

/** Descriptor Valid (Boot 1.4, 3.2.2.4, Figure 12, page 25, PDF page 31); bit 0 of 'hfidflags' */
#define XNVME_SPEC_BOOT_HOST_FABRIC_INTERFACE_HFI_DESCRIPTOR_HFIDFLAGS_DESCVLD 0x1

/**
 * HFI Transport Info Descriptor Header Template (Boot 1.4, 3.2.2.4.1, Figure 13, page 26, PDF page 32)
 */
struct __attribute__((packed)) xnvme_spec_boot_hfi_transport_info_descriptor_header_template {
	uint8_t structid; ///< Structure ID, byte 0
	uint8_t ver; ///< Version, byte 1
	uint8_t hfitt; ///< HFI Transport Type, byte 2
	uint8_t tiv; ///< Transport Info Version, byte 3
	uint16_t hfididx; ///< HFI Descriptor Index, bytes 5:4
};

/**
 * HFI Transport Info Descriptor - NVMe/TCP (Boot 1.4, 3.2.2.4.1.1, Figure 14, page 26, PDF page 32)
 */
struct __attribute__((packed)) xnvme_spec_boot_nvme_tcp {
	uint8_t structid; ///< Structure ID, byte 0
	uint8_t ver; ///< Version, byte 1
	uint8_t hfitt; ///< HFI Transport Type, byte 2
	uint8_t tiv; ///< Transport Info Version, byte 3
	uint16_t hfididx; ///< HFI Descriptor Index, bytes 5:4
	uint8_t hfitflags; ///< HFI Transport Flags, byte 6, see XNVME_SPEC_BOOT_NVME_TCP_HFITFLAGS_*
	uint32_t pcieridhfitf; ///< PCI Express Routing ID for the HFI Transport Function, bytes 10:7, see XNVME_SPEC_BOOT_NVME_TCP_PCIERIDHFITF_*
	uint8_t macaddr[6]; ///< MAC Address, bytes 16:11
	uint16_t vlanid; ///< VLAN, bytes 18:17
	uint8_t iporig; ///< IP Origin, byte 19
	uint8_t ipaddr[16]; ///< IP Address, bytes 35:20
	uint8_t ipmask; ///< Subnet Mask Prefix, byte 36
	uint8_t ipgw[16]; ///< IP Gateway, bytes 52:37
	uint8_t rsvd53; ///< Reserved, byte 53
	uint16_t rm; ///< Route Metric, bytes 55:54
	uint8_t pdns[16]; ///< Primary DNS, bytes 71:56
	uint8_t sdns[16]; ///< Secondary DNS, bytes 87:72
	uint8_t dhcps[16]; ///< DHCP Server, bytes 103:88
	uint8_t hnhor[6]; ///< Host Name Heap Object Reference, bytes 109:104
	uint8_t htieidor[6]; ///< HFI Transport Info Extended Information Descriptor Object Reference, bytes 115:110
	uint8_t pciesn; ///< PCIe Segment Number, byte 116
	uint8_t rsvd117[11]; ///< Reserved, bytes 127:117
};

/** Interface Address Autoconfigured (Boot 1.4, 3.2.2.4.1.1, Figure 14, page 26, PDF page 32); bit 3 of 'hfitflags' */
#define XNVME_SPEC_BOOT_NVME_TCP_HFITFLAGS_IAA 0x8
/** DHCP Override (Boot 1.4, 3.2.2.4.1.1, Figure 14, page 26, PDF page 32); bit 2 of 'hfitflags' */
#define XNVME_SPEC_BOOT_NVME_TCP_HFITFLAGS_DHCPO 0x4
/** Global Route vs. Link Local Override Flag (Boot 1.4, 3.2.2.4.1.1, Figure 14, page 26, PDF page 32); bit 1 of 'hfitflags' */
#define XNVME_SPEC_BOOT_NVME_TCP_HFITFLAGS_GRLLOF 0x2
/** Descriptor Valid (Boot 1.4, 3.2.2.4.1.1, Figure 14, page 26, PDF page 32); bit 0 of 'hfitflags' */
#define XNVME_SPEC_BOOT_NVME_TCP_HFITFLAGS_DESCVLD 0x1
/** Segment Group Number (Boot 1.4, 3.2.2.4.1.1, Figure 14, page 26, PDF page 32); bits 31:16 of 'pcieridhfitf' */
#define XNVME_SPEC_BOOT_NVME_TCP_PCIERIDHFITF_SGN 0xffff0000
/** Bus (Boot 1.4, 3.2.2.4.1.1, Figure 14, page 26, PDF page 32); bits 15:8 of 'pcieridhfitf' */
#define XNVME_SPEC_BOOT_NVME_TCP_PCIERIDHFITF_BN 0xff00
/** Device (Boot 1.4, 3.2.2.4.1.1, Figure 14, page 26, PDF page 32); bits 7:3 of 'pcieridhfitf' */
#define XNVME_SPEC_BOOT_NVME_TCP_PCIERIDHFITF_DN 0xf8
/** Function (Boot 1.4, 3.2.2.4.1.1, Figure 14, page 26, PDF page 32); bits 2:0 of 'pcieridhfitf' */
#define XNVME_SPEC_BOOT_NVME_TCP_PCIERIDHFITF_FN 0x7

/**
 * The values of IP Origin (Boot 1.4, 3.2.2.4.1.1, Figure 14, page 26, PDF page 32)
 */
enum xnvme_spec_boot_nvme_tcp_iporig {
	XNVME_SPEC_BOOT_NVME_TCP_IPORIG_IPPREFIXORIGINOTHER = 0x0, ///< IpPrefixOriginOther (Boot 1.4, 3.2.2.4.1.1, Figure 14, page 26, PDF page 32)
	XNVME_SPEC_BOOT_NVME_TCP_IPORIG_IPPREFIXORIGINMANUAL = 0x1, ///< IpPrefixOriginManual (Boot 1.4, 3.2.2.4.1.1, Figure 14, page 26, PDF page 32)
	XNVME_SPEC_BOOT_NVME_TCP_IPORIG_IPPREFIXORIGINDHCP = 0x3, ///< IpPrefixOriginDhcp (Boot 1.4, 3.2.2.4.1.1, Figure 14, page 26, PDF page 32)
	XNVME_SPEC_BOOT_NVME_TCP_IPORIG_IPPREFIXORIGINROUTERADVERTISEMENT = 0x4, ///< IpPrefixOriginRouterAdvertisement (Boot 1.4, 3.2.2.4.1.1, Figure 14, page 26, PDF page 32)
};

/**
 * HFI Transport Info Extended Information Descriptor (Boot 1.4, 3.2.2.4.1.3, Figure 15, page 29, PDF page 35)
 */
struct __attribute__((packed)) xnvme_spec_boot_hfi_transport_info_extended_information_descriptor {
	uint8_t structid; ///< Structure ID, byte 0
	uint8_t ver; ///< Version, byte 1
	uint16_t hfitididx; ///< HFI Transport Info Descriptor Index, bytes 3:2
	uint32_t flags; ///< Flags, bytes 7:4, see XNVME_SPEC_BOOT_HFI_TRANSPORT_INFO_EXTENDED_INFORMATION_DESCRIPTOR_FLAGS_*
	uint8_t ipdcihior[6]; ///< IPv4 and IPv6 DHCP Client Identifier Heap IAID Object Reference, bytes 13:8
	uint8_t ipdcihdor[6]; ///< IPv4 and IPv6 DHCP Client Identifier Heap DUID Object Reference, bytes 19:14
};

/** DHCP Client Identifier (Boot 1.4, 3.2.2.4.1.3, Figure 15, page 29, PDF page 35); bit 1 of 'flags' */
#define XNVME_SPEC_BOOT_HFI_TRANSPORT_INFO_EXTENDED_INFORMATION_DESCRIPTOR_FLAGS_DCI 0x2
/** Descriptor Valid (Boot 1.4, 3.2.2.4.1.3, Figure 15, page 29, PDF page 35); bit 0 of 'flags' */
#define XNVME_SPEC_BOOT_HFI_TRANSPORT_INFO_EXTENDED_INFORMATION_DESCRIPTOR_FLAGS_DESVAL 0x1

/**
 * Subsystem Namespace (SSNS) Descriptor (Boot 1.4, 3.2.2.5, Figure 17, page 30, PDF page 37)
 */
struct __attribute__((packed)) xnvme_spec_boot_subsystem_namespace_ssns_descriptor {
	uint8_t structid; ///< Structure ID, byte 0
	uint16_t ssnsdidx; ///< SSNS Descriptor Index, bytes 2:1
	uint16_t ssnsflags; ///< SSNS Flags, bytes 4:3, see XNVME_SPEC_BOOT_SUBSYSTEM_NAMESPACE_SSNS_DESCRIPTOR_SSNSFLAGS_*
	uint8_t trtype; ///< Transport Type, byte 5
	uint16_t tsflags; ///< Transport Specific Flags, bytes 7:6, see XNVME_SPEC_BOOT_SUBSYSTEM_NAMESPACE_SSNS_DESCRIPTOR_TSFLAGS_*
	uint8_t pdcidx; ///< Primary Discovery Controller Index, byte 8
	uint8_t rsvd9; ///< Reserved, byte 9
	uint8_t stahor[6]; ///< Subsystem Transport Address Heap Object Reference, bytes 15:10
	uint8_t stsihor[6]; ///< Subsystem Transport Service Identifier Heap Object Reference, bytes 21:16
	uint16_t nvmspid; ///< NVM Subsystem Port ID, bytes 23:22
	uint32_t nsid; ///< Namespace ID, bytes 27:24
	uint8_t nidt; ///< Namespace Identifier Type, byte 28
	uint8_t nid[16]; ///< Namespace Identifier, bytes 44:29
	uint8_t spdidx; ///< Security Profile Descriptor Index, byte 45
	uint8_t phfididx; ///< Primary HFI Descriptor Index, byte 46
	uint8_t rsvd47; ///< Reserved, byte 47
	uint8_t shfiahor[6]; ///< Secondary HFI Associations Heap Object Reference, bytes 53:48
	uint8_t nvmsnnqnhor[6]; ///< NVM Subsystem and Namespace NQN Heap Object Reference, bytes 59:54
	uint8_t ssnseidhor[6]; ///< SSNS Extended Information Descriptor Heap Object Reference, bytes 65:60
	uint8_t rsvd66[62]; ///< Reserved, bytes 127:66
};

/** Unavailable Namespace Flag (Boot 1.4, 3.2.2.5.1, Figure 18, page 32, PDF page 39); bits 8:7 of 'ssnsflags' */
#define XNVME_SPEC_BOOT_SUBSYSTEM_NAMESPACE_SSNS_DESCRIPTOR_SSNSFLAGS_UNFLAG 0x180
/** Discovered Namespace Flag (Boot 1.4, 3.2.2.5.1, Figure 18, page 32, PDF page 39); bit 6 of 'ssnsflags' */
#define XNVME_SPEC_BOOT_SUBSYSTEM_NAMESPACE_SSNS_DESCRIPTOR_SSNSFLAGS_DNFLAG 0x40
/** Separate Discovery Controller Flag (Boot 1.4, 3.2.2.5.1, Figure 18, page 32, PDF page 39); bit 5 of 'ssnsflags' */
#define XNVME_SPEC_BOOT_SUBSYSTEM_NAMESPACE_SSNS_DESCRIPTOR_SSNSFLAGS_SDCFLAG 0x20
/** SSNS Extended Info In-use Flag (Boot 1.4, 3.2.2.5.1, Figure 18, page 32, PDF page 39); bit 4 of 'ssnsflags' */
#define XNVME_SPEC_BOOT_SUBSYSTEM_NAMESPACE_SSNS_DESCRIPTOR_SSNSFLAGS_SSNSEIIFLAG 0x10
/** DHCP Root-Path Override Flag (Boot 1.4, 3.2.2.5.1, Figure 18, page 32, PDF page 39); bit 3 of 'ssnsflags' */
#define XNVME_SPEC_BOOT_SUBSYSTEM_NAMESPACE_SSNS_DESCRIPTOR_SSNSFLAGS_DHCPRPOFLAG 0x8
/** Use Security Flag (Boot 1.4, 3.2.2.5.1, Figure 18, page 32, PDF page 39); bit 2 of 'ssnsflags' */
#define XNVME_SPEC_BOOT_SUBSYSTEM_NAMESPACE_SSNS_DESCRIPTOR_SSNSFLAGS_USFLAG 0x4
/** Non-bootable Entry Flag (Boot 1.4, 3.2.2.5.1, Figure 18, page 32, PDF page 39); bit 1 of 'ssnsflags' */
#define XNVME_SPEC_BOOT_SUBSYSTEM_NAMESPACE_SSNS_DESCRIPTOR_SSNSFLAGS_NBEFLAG 0x2
/** Descriptor Valid (Boot 1.4, 3.2.2.5.1, Figure 18, page 32, PDF page 39); bit 0 of 'ssnsflags' */
#define XNVME_SPEC_BOOT_SUBSYSTEM_NAMESPACE_SSNS_DESCRIPTOR_SSNSFLAGS_DESCVLD 0x1
/** Data Digest Flag (Boot 1.4, 3.2.2.5.2, Figure 19, page 34, PDF page 40); bit 2 of 'tsflags' */
#define XNVME_SPEC_BOOT_SUBSYSTEM_NAMESPACE_SSNS_DESCRIPTOR_TSFLAGS_DDGSTF 0x4
/** PDU Header Digest Flag (Boot 1.4, 3.2.2.5.2, Figure 19, page 34, PDF page 40); bit 1 of 'tsflags' */
#define XNVME_SPEC_BOOT_SUBSYSTEM_NAMESPACE_SSNS_DESCRIPTOR_TSFLAGS_HDGSTF 0x2
/** Transport Specific Flags in Use (Boot 1.4, 3.2.2.5.2, Figure 19, page 34, PDF page 40); bit 0 of 'tsflags' */
#define XNVME_SPEC_BOOT_SUBSYSTEM_NAMESPACE_SSNS_DESCRIPTOR_TSFLAGS_TSFIU 0x1

/**
 * The values of Unavailable Namespace Flag (Boot 1.4, 3.2.2.5.1, Figure 18, page 32, PDF page 39)
 */
enum xnvme_spec_boot_subsystem_namespace_ssns_descriptor_ssnsflags_unflag {
	XNVME_SPEC_BOOT_SUBSYSTEM_NAMESPACE_SSNS_DESCRIPTOR_SSNSFLAGS_UNFLAG_NOT_INDICATED_BY_DRIVER = 0x0, ///< Not Indicated by Driver (Boot 1.4, 3.2.2.5.1, Figure 18, page 32, PDF page 39)
	XNVME_SPEC_BOOT_SUBSYSTEM_NAMESPACE_SSNS_DESCRIPTOR_SSNSFLAGS_UNFLAG_AVAILABLE = 0x1, ///< Available (Boot 1.4, 3.2.2.5.1, Figure 18, page 32, PDF page 39)
	XNVME_SPEC_BOOT_SUBSYSTEM_NAMESPACE_SSNS_DESCRIPTOR_SSNSFLAGS_UNFLAG_UNAVAILABLE = 0x2, ///< Unavailable (Boot 1.4, 3.2.2.5.1, Figure 18, page 32, PDF page 39)
};

/**
 * The values of Transport Type (Boot 1.4, 3.2.1.3, Figure 8, page 20, PDF page 26)
 */
enum xnvme_spec_boot_subsystem_namespace_ssns_descriptor_trtype {
	XNVME_SPEC_BOOT_SUBSYSTEM_NAMESPACE_SSNS_DESCRIPTOR_TRTYPE_NVME_TCP = 0x3, ///< NVMe/TCP (Boot 1.4, 3.2.1.3, Figure 8, page 20, PDF page 26)
};

/**
 * Subsystem and Namespace Extended Information Descriptor (Boot 1.4, 3.2.2.5.5, Figure 21, page 35, PDF page 42)
 */
struct __attribute__((packed)) xnvme_spec_boot_subsystem_and_namespace_extended_information_descriptor {
	uint8_t structid; ///< Structure ID, byte 0
	uint8_t ver; ///< Version, byte 1
	uint16_t ssnsdidx; ///< SSNS Descriptor Index, bytes 3:2
	uint32_t flags; ///< Flags, bytes 7:4, see XNVME_SPEC_BOOT_SUBSYSTEM_AND_NAMESPACE_EXTENDED_INFORMATION_DESCRIPTOR_FLAGS_*
	uint16_t ctrlid; ///< Controller ID, bytes 9:8
	uint16_t asqsz; ///< Admin Submission Queue Size, bytes 11:10
	uint8_t dhcprpshor[6]; ///< DHCP Root Path String Heap Object Reference, bytes 17:12
	uint8_t naed; ///< Namespace Availability Enhanced Diagnostic, byte 18
	uint8_t cipeec; ///< Connect Invalid Parameters Extended Error Codes, byte 19
	uint16_t cto; ///< Connection Timeout, bytes 21:20
	uint8_t nceec; ///< Network and Connection Extended Error Codes, byte 22
};

/** Administrative ASQSZ (Boot 1.4, 3.2.2.5.5, Figure 21, page 35, PDF page 42); bit 1 of 'flags' */
#define XNVME_SPEC_BOOT_SUBSYSTEM_AND_NAMESPACE_EXTENDED_INFORMATION_DESCRIPTOR_FLAGS_AASQZ 0x2
/** Descriptor Valid (Boot 1.4, 3.2.2.5.5, Figure 21, page 35, PDF page 42); bit 0 of 'flags' */
#define XNVME_SPEC_BOOT_SUBSYSTEM_AND_NAMESPACE_EXTENDED_INFORMATION_DESCRIPTOR_FLAGS_DESCVLD 0x1

/**
 * The values of Namespace Availability Enhanced Diagnostic (Boot 1.4, 3.2.2.5.5, Figure 21, page 35, PDF page 42)
 */
enum xnvme_spec_boot_subsystem_and_namespace_extended_information_descriptor_naed {
	XNVME_SPEC_BOOT_SUBSYSTEM_AND_NAMESPACE_EXTENDED_INFORMATION_DESCRIPTOR_NAED_NETWORK_ERROR = 0x1, ///< Network Error (Boot 1.4, 3.2.2.5.5, Figure 21, page 35, PDF page 42)
	XNVME_SPEC_BOOT_SUBSYSTEM_AND_NAMESPACE_EXTENDED_INFORMATION_DESCRIPTOR_NAED_CONNECTION_FAILURE = 0x2, ///< Connection Failure (Boot 1.4, 3.2.2.5.5, Figure 21, page 35, PDF page 42)
	XNVME_SPEC_BOOT_SUBSYSTEM_AND_NAMESPACE_EXTENDED_INFORMATION_DESCRIPTOR_NAED_CONNECT_INVALID_PARAMETERS = 0x3, ///< Connect Invalid Parameters (Boot 1.4, 3.2.2.5.5, Figure 21, page 35, PDF page 42)
	XNVME_SPEC_BOOT_SUBSYSTEM_AND_NAMESPACE_EXTENDED_INFORMATION_DESCRIPTOR_NAED_CONNECT_INVALID_HOST = 0x4, ///< Connect Invalid Host (Boot 1.4, 3.2.2.5.5, Figure 21, page 35, PDF page 42)
	XNVME_SPEC_BOOT_SUBSYSTEM_AND_NAMESPACE_EXTENDED_INFORMATION_DESCRIPTOR_NAED_CONNECTION_TIMEOUT = 0x5, ///< Connection Timeout (Boot 1.4, 3.2.2.5.5, Figure 21, page 35, PDF page 42)
	XNVME_SPEC_BOOT_SUBSYSTEM_AND_NAMESPACE_EXTENDED_INFORMATION_DESCRIPTOR_NAED_NAMESPACE_MISSING = 0x7, ///< Namespace Missing (Boot 1.4, 3.2.2.5.5, Figure 21, page 35, PDF page 42)
	XNVME_SPEC_BOOT_SUBSYSTEM_AND_NAMESPACE_EXTENDED_INFORMATION_DESCRIPTOR_NAED_NAMESPACE_NOT_READY = 0x8, ///< Namespace Not Ready (Boot 1.4, 3.2.2.5.5, Figure 21, page 35, PDF page 42)
	XNVME_SPEC_BOOT_SUBSYSTEM_AND_NAMESPACE_EXTENDED_INFORMATION_DESCRIPTOR_NAED_REDFISH_SECRET_KEYPATH_OBJECT = 0x9, ///< Redfish Secret Keypath Object (Boot 1.4, 3.2.2.5.5, Figure 21, page 35, PDF page 42)
	XNVME_SPEC_BOOT_SUBSYSTEM_AND_NAMESPACE_EXTENDED_INFORMATION_DESCRIPTOR_NAED_SECURE_CHANNEL_NEGOTIATION = 0x10, ///< Secure Channel Negotiation (Boot 1.4, 3.2.2.5.5, Figure 21, page 35, PDF page 42)
	XNVME_SPEC_BOOT_SUBSYSTEM_AND_NAMESPACE_EXTENDED_INFORMATION_DESCRIPTOR_NAED_AUTHENTICATION_FAILURE = 0x11, ///< Authentication Failure (Boot 1.4, 3.2.2.5.5, Figure 21, page 35, PDF page 42)
	XNVME_SPEC_BOOT_SUBSYSTEM_AND_NAMESPACE_EXTENDED_INFORMATION_DESCRIPTOR_NAED_AUTHENTICATION_REQUIRED = 0x12, ///< Authentication Required (Boot 1.4, 3.2.2.5.5, Figure 21, page 35, PDF page 42)
	XNVME_SPEC_BOOT_SUBSYSTEM_AND_NAMESPACE_EXTENDED_INFORMATION_DESCRIPTOR_NAED_DHCP_FAILURE = 0x13, ///< DHCP Failure (Boot 1.4, 3.2.2.5.5, Figure 21, page 35, PDF page 42)
	XNVME_SPEC_BOOT_SUBSYSTEM_AND_NAMESPACE_EXTENDED_INFORMATION_DESCRIPTOR_NAED_SNSS_DHCP_ROOT_PATH = 0x14, ///< SNSS DHCP Root-Path (Boot 1.4, 3.2.2.5.5, Figure 21, page 35, PDF page 42)
};

/**
 * The values of Connect Invalid Parameters Extended Error Codes (Boot 1.4, 3.2.2.5.5, Figure 21, page 35, PDF page 42)
 */
enum xnvme_spec_boot_subsystem_and_namespace_extended_information_descriptor_cipeec {
	XNVME_SPEC_BOOT_SUBSYSTEM_AND_NAMESPACE_EXTENDED_INFORMATION_DESCRIPTOR_CIPEEC_NO_EXTENDED_ERROR_KNOWN = 0x0, ///< No Extended Error Known (Boot 1.4, 3.2.2.5.5, Figure 21, page 35, PDF page 42)
	XNVME_SPEC_BOOT_SUBSYSTEM_AND_NAMESPACE_EXTENDED_INFORMATION_DESCRIPTOR_CIPEEC_GENERAL_PARAMETERS_FAILURE = 0x1, ///< General Parameters Failure (Boot 1.4, 3.2.2.5.5, Figure 21, page 35, PDF page 42)
	XNVME_SPEC_BOOT_SUBSYSTEM_AND_NAMESPACE_EXTENDED_INFORMATION_DESCRIPTOR_CIPEEC_DIFFE_HELLMAN_MISMATCH = 0x3, ///< Diffe-Hellman Mismatch (Boot 1.4, 3.2.2.5.5, Figure 21, page 35, PDF page 42)
	XNVME_SPEC_BOOT_SUBSYSTEM_AND_NAMESPACE_EXTENDED_INFORMATION_DESCRIPTOR_CIPEEC_INVALID_CONTROLLER_ID = 0x4, ///< Invalid Controller ID (Boot 1.4, 3.2.2.5.5, Figure 21, page 35, PDF page 42)
	XNVME_SPEC_BOOT_SUBSYSTEM_AND_NAMESPACE_EXTENDED_INFORMATION_DESCRIPTOR_CIPEEC_IO_CONTROLLER_DISABLED = 0x5, ///< I/O Controller Disabled (Boot 1.4, 3.2.2.5.5, Figure 21, page 35, PDF page 42)
};

/**
 * The values of Network and Connection Extended Error Codes (Boot 1.4, 3.2.2.5.5, Figure 21, page 35, PDF page 42)
 */
enum xnvme_spec_boot_subsystem_and_namespace_extended_information_descriptor_nceec {
	XNVME_SPEC_BOOT_SUBSYSTEM_AND_NAMESPACE_EXTENDED_INFORMATION_DESCRIPTOR_NCEEC_NO_EXTENDED_ERROR_KNOWN = 0x0, ///< No Extended Error Known (Boot 1.4, 3.2.2.5.5, Figure 21, page 35, PDF page 42)
	XNVME_SPEC_BOOT_SUBSYSTEM_AND_NAMESPACE_EXTENDED_INFORMATION_DESCRIPTOR_NCEEC_LINK_ERROR = 0x1, ///< Link Error (Boot 1.4, 3.2.2.5.5, Figure 21, page 35, PDF page 42)
	XNVME_SPEC_BOOT_SUBSYSTEM_AND_NAMESPACE_EXTENDED_INFORMATION_DESCRIPTOR_NCEEC_CARRIER_ERROR = 0x2, ///< Carrier Error (Boot 1.4, 3.2.2.5.5, Figure 21, page 35, PDF page 42)
	XNVME_SPEC_BOOT_SUBSYSTEM_AND_NAMESPACE_EXTENDED_INFORMATION_DESCRIPTOR_NCEEC_CONNECTION_REFUSED = 0x3, ///< Connection Refused (Boot 1.4, 3.2.2.5.5, Figure 21, page 35, PDF page 42)
	XNVME_SPEC_BOOT_SUBSYSTEM_AND_NAMESPACE_EXTENDED_INFORMATION_DESCRIPTOR_NCEEC_NO_ROUTE_TO_HOST = 0x4, ///< No Route to Host (Boot 1.4, 3.2.2.5.5, Figure 21, page 35, PDF page 42)
	XNVME_SPEC_BOOT_SUBSYSTEM_AND_NAMESPACE_EXTENDED_INFORMATION_DESCRIPTOR_NCEEC_PACKET_FRAGMENTATION_ERROR = 0x5, ///< Packet Fragmentation Error (Boot 1.4, 3.2.2.5.5, Figure 21, page 35, PDF page 42)
	XNVME_SPEC_BOOT_SUBSYSTEM_AND_NAMESPACE_EXTENDED_INFORMATION_DESCRIPTOR_NCEEC_MTU_MISMATCH = 0x6, ///< MTU Mismatch (Boot 1.4, 3.2.2.5.5, Figure 21, page 35, PDF page 42)
	XNVME_SPEC_BOOT_SUBSYSTEM_AND_NAMESPACE_EXTENDED_INFORMATION_DESCRIPTOR_NCEEC_NAME_RESOLUTION_FAILURE = 0x7, ///< Name Resolution Failure (Boot 1.4, 3.2.2.5.5, Figure 21, page 35, PDF page 42)
};

/**
 * Security Profile Descriptor (Boot 1.4, 3.2.2.6, Figure 23, page 38, PDF page 44)
 */
struct __attribute__((packed)) xnvme_spec_boot_security_profile_descriptor {
	uint8_t structid; ///< Structure ID, byte 0
	uint8_t spdidx; ///< Security Profile Descriptor Index, byte 1
	uint16_t spdflags; ///< Security Profile Descriptor Flags, bytes 3:2, see XNVME_SPEC_BOOT_SECURITY_PROFILE_DESCRIPTOR_SPDFLAGS_*
	uint8_t st; ///< Secret Type, byte 4, see XNVME_SPEC_BOOT_SECURITY_PROFILE_DESCRIPTOR_ST_*
	uint8_t rsvd5; ///< Reserved, byte 5
	uint8_t scahor[6]; ///< Secure Channel Algorithm Heap Object Reference, bytes 11:6
	uint8_t aphor[6]; ///< Authentication Protocols Heap Object Reference, bytes 17:12
	uint8_t csohor[6]; ///< Cipher Suite Offset Heap Object Reference, bytes 23:18
	uint8_t kxghor[6]; ///< KX Groups Heap Object Reference, bytes 29:24
	uint8_t shfohor[6]; ///< Secure Hash Functions Offset Heap Object Reference, bytes 35:30
	uint8_t skohor[6]; ///< Secret Keypath Offset Heap Object Reference, bytes 41:36
	uint8_t rsvd42[22]; ///< Reserved, bytes 63:42
};

/** Secure Hash Functions Policy List (Boot 1.4, 3.2.2.6, Figure 24, page 40, PDF page 46); bit 11 of 'spdflags' */
#define XNVME_SPEC_BOOT_SECURITY_PROFILE_DESCRIPTOR_SPDFLAGS_SHFPL 0x800
/** Authentication KX Groups Restricted by Policy List (Boot 1.4, 3.2.2.6, Figure 24, page 40, PDF page 46); bit 10 of 'spdflags' */
#define XNVME_SPEC_BOOT_SECURITY_PROFILE_DESCRIPTOR_SPDFLAGS_AKXGRPL 0x400
/** Cipher Suites Restricted by Policy (Boot 1.4, 3.2.2.6, Figure 24, page 40, PDF page 46); bit 9 of 'spdflags' */
#define XNVME_SPEC_BOOT_SECURITY_PROFILE_DESCRIPTOR_SPDFLAGS_CSRP 0x200
/** Security Policy List (Boot 1.4, 3.2.2.6, Figure 24, page 40, PDF page 46); bits 8:7 of 'spdflags' */
#define XNVME_SPEC_BOOT_SECURITY_PROFILE_DESCRIPTOR_SPDFLAGS_SPL 0x180
/** Secure Channel Negotiation Required (Boot 1.4, 3.2.2.6, Figure 24, page 40, PDF page 46); bits 6:5 of 'spdflags' */
#define XNVME_SPEC_BOOT_SECURITY_PROFILE_DESCRIPTOR_SPDFLAGS_SCNR 0x60
/** Authentication Policy List (Boot 1.4, 3.2.2.6, Figure 24, page 40, PDF page 46); bits 4:3 of 'spdflags' */
#define XNVME_SPEC_BOOT_SECURITY_PROFILE_DESCRIPTOR_SPDFLAGS_APL 0x18
/** In-Band Authentication Required (Boot 1.4, 3.2.2.6, Figure 24, page 40, PDF page 46); bits 2:1 of 'spdflags' */
#define XNVME_SPEC_BOOT_SECURITY_PROFILE_DESCRIPTOR_SPDFLAGS_IBAR 0x6
/** Descriptor Valid (Boot 1.4, 3.2.2.6, Figure 24, page 40, PDF page 46); bit 0 of 'spdflags' */
#define XNVME_SPEC_BOOT_SECURITY_PROFILE_DESCRIPTOR_SPDFLAGS_DESCVLD 0x1
/** Redfish Host Interface URI (Boot 1.4, 3.2.2.6, Figure 23, page 38, PDF page 44); bit 1 of 'st' */
#define XNVME_SPEC_BOOT_SECURITY_PROFILE_DESCRIPTOR_ST_RHIURH 0x2

/**
 * Discovery Descriptor (Boot 1.4, 3.2.2.7, Figure 26, page 42, PDF page 48)
 */
struct __attribute__((packed)) xnvme_spec_boot_discovery_descriptor {
	uint8_t structid; ///< Structure ID, byte 0
	uint8_t ddflags; ///< Discovery Descriptor Flags, byte 1, see XNVME_SPEC_BOOT_DISCOVERY_DESCRIPTOR_DDFLAGS_*
	uint8_t ddidx; ///< Discovery Descriptor Index, byte 2
	uint8_t hfididx; ///< HFI Descriptor Index, byte 3
	uint8_t spdidx; ///< Security Profile Descriptor Index, byte 4
	uint8_t rsvd5; ///< Reserved, byte 5
	uint8_t dcahor[6]; ///< Discovery Controller Address Heap Object Reference, bytes 11:6
	uint8_t dcnqnhor[6]; ///< Discovery Controller NQN Heap Object Reference, bytes 17:12
	uint8_t rsvd18[14]; ///< Reserved, bytes 31:18
};

/** Descriptor Valid (Boot 1.4, 3.2.2.7, Figure 26, page 42, PDF page 48); bit 0 of 'ddflags' */
#define XNVME_SPEC_BOOT_DISCOVERY_DESCRIPTOR_DDFLAGS_DESCVLD 0x1

#endif /* LIBXNVME_SPEC_BOOT_H */
