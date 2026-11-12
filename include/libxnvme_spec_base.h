/**
 * What the base specification defines, by xnvme_spec
 *
 * Written from the data model of senfd 0.19.0, by toolbox/xnvme_spec_headers.py;
 * each name and mask is as the figure it cites gives it. Do not
 * edit: see the contributor docs on producing the spec headers.
 *
 * @file libxnvme_spec_base.h
 */
#ifndef LIBXNVME_SPEC_BASE_H
#define LIBXNVME_SPEC_BASE_H

#include <stdint.h>

/**
 * The opcodes of the Admin commands
 */
enum xnvme_spec_admin_opcode {
	XNVME_SPEC_ADMIN_DELETE_IO_SUBMISSION_QUEUE_OPCODE = 0x0, ///< Delete I/O Submission Queue (Base 2.4, 5, Figure 143, page 176, PDF page 202)
	XNVME_SPEC_ADMIN_CREATE_IO_SUBMISSION_QUEUE_OPCODE = 0x1, ///< Create I/O Submission Queue (Base 2.4, 5, Figure 143, page 176, PDF page 202)
	XNVME_SPEC_ADMIN_GET_LOG_PAGE_OPCODE = 0x2, ///< Get Log Page (Base 2.4, 5, Figure 143, page 176, PDF page 202)
	XNVME_SPEC_ADMIN_DELETE_IO_COMPLETION_QUEUE_OPCODE = 0x4, ///< Delete I/O Completion Queue (Base 2.4, 5, Figure 143, page 176, PDF page 202)
	XNVME_SPEC_ADMIN_CREATE_IO_COMPLETION_QUEUE_OPCODE = 0x5, ///< Create I/O Completion Queue (Base 2.4, 5, Figure 143, page 176, PDF page 202)
	XNVME_SPEC_ADMIN_IDENTIFY_OPCODE = 0x6, ///< Identify (Base 2.4, 5, Figure 143, page 176, PDF page 202)
	XNVME_SPEC_ADMIN_ABORT_OPCODE = 0x8, ///< Abort (Base 2.4, 5, Figure 143, page 176, PDF page 202)
	XNVME_SPEC_ADMIN_SET_FEATURES_OPCODE = 0x9, ///< Set Features (Base 2.4, 5, Figure 143, page 176, PDF page 202)
	XNVME_SPEC_ADMIN_GET_FEATURES_OPCODE = 0xa, ///< Get Features (Base 2.4, 5, Figure 143, page 176, PDF page 202)
	XNVME_SPEC_ADMIN_ASYNCHRONOUS_EVENT_REQUEST_OPCODE = 0xc, ///< Asynchronous Event Request (Base 2.4, 5, Figure 143, page 176, PDF page 202)
	XNVME_SPEC_ADMIN_NAMESPACE_MANAGEMENT_OPCODE = 0xd, ///< Namespace Management (Base 2.4, 5, Figure 143, page 176, PDF page 202)
	XNVME_SPEC_ADMIN_FIRMWARE_COMMIT_OPCODE = 0x10, ///< Firmware Commit (Base 2.4, 5, Figure 143, page 176, PDF page 202)
	XNVME_SPEC_ADMIN_FIRMWARE_IMAGE_DOWNLOAD_OPCODE = 0x11, ///< Firmware Image Download (Base 2.4, 5, Figure 143, page 176, PDF page 202)
	XNVME_SPEC_ADMIN_DEVICE_SELF_TEST_OPCODE = 0x14, ///< Device Self-test (Base 2.4, 5, Figure 143, page 176, PDF page 202)
	XNVME_SPEC_ADMIN_NAMESPACE_ATTACHMENT_OPCODE = 0x15, ///< Namespace Attachment (Base 2.4, 5, Figure 143, page 176, PDF page 202)
	XNVME_SPEC_ADMIN_KEEP_ALIVE_OPCODE = 0x18, ///< Keep Alive (Base 2.4, 5, Figure 143, page 176, PDF page 202)
	XNVME_SPEC_ADMIN_DIRECTIVE_SEND_OPCODE = 0x19, ///< Directive Send (Base 2.4, 5, Figure 143, page 176, PDF page 202)
	XNVME_SPEC_ADMIN_DIRECTIVE_RECEIVE_OPCODE = 0x1a, ///< Directive Receive (Base 2.4, 5, Figure 143, page 176, PDF page 202)
	XNVME_SPEC_ADMIN_VIRTUALIZATION_MANAGEMENT_OPCODE = 0x1c, ///< Virtualization Management (Base 2.4, 5, Figure 143, page 176, PDF page 202)
	XNVME_SPEC_ADMIN_NVME_MI_SEND_OPCODE = 0x1d, ///< NVMe-MI Send (Base 2.4, 5, Figure 143, page 176, PDF page 202)
	XNVME_SPEC_ADMIN_NVME_MI_RECEIVE_OPCODE = 0x1e, ///< NVMe-MI Receive (Base 2.4, 5, Figure 143, page 176, PDF page 202)
	XNVME_SPEC_ADMIN_CAPACITY_MANAGEMENT_OPCODE = 0x20, ///< Capacity Management (Base 2.4, 5, Figure 143, page 176, PDF page 202)
	XNVME_SPEC_ADMIN_DISCOVERY_INFORMATION_MANAGEMENT_OPCODE = 0x21, ///< Discovery Information Management (Base 2.4, 5, Figure 143, page 176, PDF page 202)
	XNVME_SPEC_ADMIN_FABRIC_ZONING_RECEIVE_OPCODE = 0x22, ///< Fabric Zoning Receive (Base 2.4, 5, Figure 143, page 176, PDF page 202)
	XNVME_SPEC_ADMIN_LOCKDOWN_OPCODE = 0x24, ///< Lockdown (Base 2.4, 5, Figure 143, page 176, PDF page 202)
	XNVME_SPEC_ADMIN_FABRIC_ZONING_LOOKUP_OPCODE = 0x25, ///< Fabric Zoning Lookup (Base 2.4, 5, Figure 143, page 176, PDF page 202)
	XNVME_SPEC_ADMIN_CLEAR_EXPORTED_NVM_RESOURCE_CONFIGURATION_OPCODE = 0x28, ///< Clear Exported NVM Resource Configuration (Base 2.4, 5, Figure 143, page 176, PDF page 202)
	XNVME_SPEC_ADMIN_FABRIC_ZONING_SEND_OPCODE = 0x29, ///< Fabric Zoning Send (Base 2.4, 5, Figure 143, page 176, PDF page 202)
	XNVME_SPEC_ADMIN_MANAGE_EXPORTED_NVM_SUBSYSTEM_RECEIVE_OPCODE = 0x2a, ///< Manage Exported NVM Subsystem Receive (Base 2.4, 5, Figure 143, page 176, PDF page 202)
	XNVME_SPEC_ADMIN_MANAGE_EXPORTED_NVM_SUBSYSTEM_SEND_OPCODE = 0x2d, ///< Manage Exported NVM Subsystem Send (Base 2.4, 5, Figure 143, page 176, PDF page 202)
	XNVME_SPEC_ADMIN_MANAGE_EXPORTED_NAMESPACE_OPCODE = 0x31, ///< Manage Exported Namespace (Base 2.4, 5, Figure 143, page 176, PDF page 202)
	XNVME_SPEC_ADMIN_MANAGE_EXPORTED_PORT_OPCODE = 0x35, ///< Manage Exported Port (Base 2.4, 5, Figure 143, page 176, PDF page 202)
	XNVME_SPEC_ADMIN_CROSS_CONTROLLER_RESET_OPCODE = 0x38, ///< Cross-Controller Reset (Base 2.4, 5, Figure 143, page 176, PDF page 202)
	XNVME_SPEC_ADMIN_SEND_DISCOVERY_LOG_PAGE_OPCODE = 0x39, ///< Send Discovery Log Page (Base 2.4, 5, Figure 143, page 176, PDF page 202)
	XNVME_SPEC_ADMIN_TRACK_SEND_OPCODE = 0x3d, ///< Track Send (Base 2.4, 5, Figure 143, page 176, PDF page 202)
	XNVME_SPEC_ADMIN_TRACK_RECEIVE_OPCODE = 0x3e, ///< Track Receive (Base 2.4, 5, Figure 143, page 176, PDF page 202)
	XNVME_SPEC_ADMIN_MIGRATION_SEND_OPCODE = 0x41, ///< Migration Send (Base 2.4, 5, Figure 143, page 176, PDF page 202)
	XNVME_SPEC_ADMIN_MIGRATION_RECEIVE_OPCODE = 0x42, ///< Migration Receive (Base 2.4, 5, Figure 143, page 176, PDF page 202)
	XNVME_SPEC_ADMIN_CONTROLLER_DATA_QUEUE_OPCODE = 0x45, ///< Controller Data Queue (Base 2.4, 5, Figure 143, page 176, PDF page 202)
	XNVME_SPEC_ADMIN_DOORBELL_BUFFER_CONFIG_OPCODE = 0x7c, ///< Doorbell Buffer Config (Base 2.4, 5, Figure 143, page 176, PDF page 202)
	XNVME_SPEC_ADMIN_FORMAT_NVM_OPCODE = 0x80, ///< Format NVM (Base 2.4, 5, Figure 143, page 176, PDF page 202)
	XNVME_SPEC_ADMIN_SECURITY_SEND_OPCODE = 0x81, ///< Security Send (Base 2.4, 5, Figure 143, page 176, PDF page 202)
	XNVME_SPEC_ADMIN_SECURITY_RECEIVE_OPCODE = 0x82, ///< Security Receive (Base 2.4, 5, Figure 143, page 176, PDF page 202)
	XNVME_SPEC_ADMIN_SANITIZE_OPCODE = 0x84, ///< Sanitize (Base 2.4, 5, Figure 143, page 176, PDF page 202)
	XNVME_SPEC_ADMIN_LOAD_PROGRAM_OPCODE = 0x85, ///< Load Program (Base 2.4, 5, Figure 143, page 176, PDF page 202)
	XNVME_SPEC_ADMIN_GET_LBA_STATUS_OPCODE = 0x86, ///< Get LBA Status (Base 2.4, 5, Figure 143, page 176, PDF page 202)
	XNVME_SPEC_ADMIN_PROGRAM_ACTIVATION_MANAGEMENT_OPCODE = 0x88, ///< Program Activation Management (Base 2.4, 5, Figure 143, page 176, PDF page 202)
	XNVME_SPEC_ADMIN_MEMORY_RANGE_SET_MANAGEMENT_OPCODE = 0x89, ///< Memory Range Set Management (Base 2.4, 5, Figure 143, page 176, PDF page 202)
	XNVME_SPEC_ADMIN_SANITIZE_NAMESPACE_OPCODE = 0x8c, ///< Sanitize Namespace (Base 2.4, 5, Figure 143, page 176, PDF page 202)
};

/**
 * The opcodes of the I/O commands
 */
enum xnvme_spec_io_opcode {
	XNVME_SPEC_IO_FLUSH_OPCODE = 0x0, ///< Flush (Base 2.4, 7, Figure 645, page 560, PDF page 590)
	XNVME_SPEC_IO_RESERVATION_REGISTER_OPCODE = 0xd, ///< Reservation Register (Base 2.4, 7, Figure 645, page 560, PDF page 590)
	XNVME_SPEC_IO_RESERVATION_REPORT_OPCODE = 0xe, ///< Reservation Report (Base 2.4, 7, Figure 645, page 560, PDF page 590)
	XNVME_SPEC_IO_RESERVATION_ACQUIRE_OPCODE = 0x11, ///< Reservation Acquire (Base 2.4, 7, Figure 645, page 560, PDF page 590)
	XNVME_SPEC_IO_IO_MANAGEMENT_RECEIVE_OPCODE = 0x12, ///< I/O Management Receive (Base 2.4, 7, Figure 645, page 560, PDF page 590)
	XNVME_SPEC_IO_RESERVATION_RELEASE_OPCODE = 0x15, ///< Reservation Release (Base 2.4, 7, Figure 645, page 560, PDF page 590)
	XNVME_SPEC_IO_CANCEL_OPCODE = 0x18, ///< Cancel (Base 2.4, 7, Figure 645, page 560, PDF page 590)
	XNVME_SPEC_IO_IO_MANAGEMENT_SEND_OPCODE = 0x1d, ///< I/O Management Send (Base 2.4, 7, Figure 645, page 560, PDF page 590)
};

/**
 * The identifiers of the features
 */
enum xnvme_spec_base_feature_identifier {
	XNVME_SPEC_BASE_FEATURE_ARBITRATION = 0x1, ///< Arbitration (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
	XNVME_SPEC_BASE_FEATURE_POWER_MANAGEMENT = 0x2, ///< Power Management (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
	XNVME_SPEC_BASE_FEATURE_LBA_RANGE_TYPE = 0x3, ///< LBA Range Type (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
	XNVME_SPEC_BASE_FEATURE_TEMPERATURE_THRESHOLD = 0x4, ///< Temperature Threshold (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
	XNVME_SPEC_BASE_FEATURE_ERROR_RECOVERY = 0x5, ///< Error Recovery (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
	XNVME_SPEC_BASE_FEATURE_VOLATILE_WRITE_CACHED = 0x6, ///< Volatile Write Cached (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
	XNVME_SPEC_BASE_FEATURE_NUMBER_OF_QUEUES = 0x7, ///< Number of Queues (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
	XNVME_SPEC_BASE_FEATURE_INTERRUPT_COALESCING = 0x8, ///< Interrupt Coalescing (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
	XNVME_SPEC_BASE_FEATURE_INTERRUPT_VECTOR_CONFIGURATION = 0x9, ///< Interrupt Vector Configuration (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
	XNVME_SPEC_BASE_FEATURE_WRITE_ATOMICITY_NORMAL = 0xa, ///< Write Atomicity Normal (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
	XNVME_SPEC_BASE_FEATURE_ASYNCHRONOUS_EVENT_CONFIGURATION = 0xb, ///< Asynchronous Event Configuration (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
	XNVME_SPEC_BASE_FEATURE_AUTONOMOUS_POWER_STATE_TRANSITION = 0xc, ///< Autonomous Power State Transition (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
	XNVME_SPEC_BASE_FEATURE_HOST_MEMORY_BUFFER = 0xd, ///< Host Memory Buffer (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
	XNVME_SPEC_BASE_FEATURE_TIMESTAMP = 0xe, ///< Timestamp (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
	XNVME_SPEC_BASE_FEATURE_KEEP_ALIVE_TIMER = 0xf, ///< Keep Alive Timer (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
	XNVME_SPEC_BASE_FEATURE_HOST_CONTROLLED_THERMAL_MANAGEMENT = 0x10, ///< Host Controlled Thermal Management (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
	XNVME_SPEC_BASE_FEATURE_NON_OPERATIONAL_POWER_STATE_CONFIG = 0x11, ///< Non-Operational Power State Config (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
	XNVME_SPEC_BASE_FEATURE_READ_RECOVERY_LEVEL_CONFIG = 0x12, ///< Read Recovery Level Config (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
	XNVME_SPEC_BASE_FEATURE_PREDICTABLE_LATENCY_MODE_CONFIG = 0x13, ///< Predictable Latency Mode Config (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
	XNVME_SPEC_BASE_FEATURE_PREDICTABLE_LATENCY_MODE_WINDOW = 0x14, ///< Predictable Latency Mode Window (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
	XNVME_SPEC_BASE_FEATURE_LBA_STATUS_INFORMATION_REPORT_INTERVAL = 0x15, ///< LBA Status Information Report Interval (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
	XNVME_SPEC_BASE_FEATURE_HOST_BEHAVIOR_SUPPORT = 0x16, ///< Host Behavior Support (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
	XNVME_SPEC_BASE_FEATURE_SANITIZE_CONFIG = 0x17, ///< Sanitize Config (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
	XNVME_SPEC_BASE_FEATURE_ENDURANCE_GROUP_EVENT_CONFIGURATION = 0x18, ///< Endurance Group Event Configuration (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
	XNVME_SPEC_BASE_FEATURE_IO_COMMAND_SET_PROFILE = 0x19, ///< I/O Command Set Profile (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
	XNVME_SPEC_BASE_FEATURE_SPINUP_CONTROL = 0x1a, ///< Spinup Control (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
	XNVME_SPEC_BASE_FEATURE_POWER_LOSS_SIGNALING_CONFIG = 0x1b, ///< Power Loss Signaling Config (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
	XNVME_SPEC_BASE_FEATURE_PERFORMANCE_CHARACTERISTICS = 0x1c, ///< Performance Characteristics (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
	XNVME_SPEC_BASE_FEATURE_FLEXIBLE_DATA_PLACEMENT = 0x1d, ///< Flexible Data Placement (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
	XNVME_SPEC_BASE_FEATURE_FLEXIBLE_DATA_PLACEMENT_EVENTS = 0x1e, ///< Flexible Data Placement Events (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
	XNVME_SPEC_BASE_FEATURE_NAMESPACE_ADMIN_LABEL = 0x1f, ///< Namespace Admin Label (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
	XNVME_SPEC_BASE_FEATURE_KEY_VALUE_CONFIGURATION = 0x20, ///< Key Value Configuration (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
	XNVME_SPEC_BASE_FEATURE_CONTROLLER_DATA_QUEUE = 0x21, ///< Controller Data Queue (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
	XNVME_SPEC_BASE_FEATURE_CONFIGURABLE_DEVICE_PERSONALITY = 0x22, ///< Configurable Device Personality (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
	XNVME_SPEC_BASE_FEATURE_POWER_LIMIT = 0x23, ///< Power Limit (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
	XNVME_SPEC_BASE_FEATURE_POWER_THRESHOLD = 0x24, ///< Power Threshold (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
	XNVME_SPEC_BASE_FEATURE_POWER_MEASUREMENT = 0x25, ///< Power Measurement (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
	XNVME_SPEC_BASE_FEATURE_VOLTAGE_THRESHOLD = 0x26, ///< Voltage Threshold (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
	XNVME_SPEC_BASE_FEATURE_VOLTAGE_MEASUREMENT = 0x27, ///< Voltage Measurement (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
	XNVME_SPEC_BASE_FEATURE_RATE_LIMITING = 0x28, ///< Rate Limiting (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
	XNVME_SPEC_BASE_FEATURE_EMBEDDED_MANAGEMENT_CONTROLLER_ADDRESS = 0x78, ///< Embedded Management Controller Address (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
	XNVME_SPEC_BASE_FEATURE_HOST_MANAGEMENT_AGENT_ADDRESS = 0x79, ///< Host Management Agent Address (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
	XNVME_SPEC_BASE_FEATURE_ENHANCED_CONTROLLER_METADATA = 0x7d, ///< Enhanced Controller Metadata (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
	XNVME_SPEC_BASE_FEATURE_CONTROLLER_METADATA = 0x7e, ///< Controller Metadata (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
	XNVME_SPEC_BASE_FEATURE_NAMESPACE_METADATA = 0x7f, ///< Namespace Metadata (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
	XNVME_SPEC_BASE_FEATURE_SOFTWARE_PROGRESS_MARKER = 0x80, ///< Software Progress Marker (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
	XNVME_SPEC_BASE_FEATURE_HOST_IDENTIFIER = 0x81, ///< Host Identifier (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
	XNVME_SPEC_BASE_FEATURE_RESERVATION_NOTIFICATION_MASK = 0x82, ///< Reservation Notification Mask (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
	XNVME_SPEC_BASE_FEATURE_RESERVATION_PERSISTENCE = 0x83, ///< Reservation Persistence (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
	XNVME_SPEC_BASE_FEATURE_NAMESPACE_WRITE_PROTECTION_CONFIG = 0x84, ///< Namespace Write Protection Config (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
	XNVME_SPEC_BASE_FEATURE_BOOT_PARTITION_WRITE_PROTECTION_CONFIG = 0x85, ///< Boot Partition Write Protection Config (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
};

/**
 * The identifiers of the log pages
 */
enum xnvme_spec_base_log_page_identifier {
	XNVME_SPEC_BASE_LOG_PAGE_SUPPORTED_LOG_PAGES = 0x0, ///< Supported Log Pages (Base 2.4, 3.1.3.5, Figure 31, page 48, PDF page 74)
	XNVME_SPEC_BASE_LOG_PAGE_ERROR_INFORMATION = 0x1, ///< Error Information (Base 2.4, 3.1.3.5, Figure 31, page 48, PDF page 74)
	XNVME_SPEC_BASE_LOG_PAGE_SMART_HEALTH_INFORMATION_CONTROLLER_SCOPE = 0x2, ///< SMART / Health Information (Controller scope) (Base 2.4, 3.1.3.5, Figure 31, page 48, PDF page 74)
	XNVME_SPEC_BASE_LOG_PAGE_SMART_HEALTH_INFORMATION_NAMESPACE_SCOPE = 0x2, ///< SMART / Health Information (Namespace scope) (Base 2.4, 3.1.3.5, Figure 31, page 48, PDF page 74)
	XNVME_SPEC_BASE_LOG_PAGE_SMART_HEALTH_INFORMATION = 0x2, ///< SMART / Health Information (Base 2.4, 5.2.13, Figure 209, page 215, PDF page 241)
	XNVME_SPEC_BASE_LOG_PAGE_FIRMWARE_SLOT_INFORMATION = 0x3, ///< Firmware Slot Information (Base 2.4, 3.1.3.5, Figure 31, page 48, PDF page 74)
	XNVME_SPEC_BASE_LOG_PAGE_CHANGED_ATTACHED_NAMESPACE_LIST = 0x4, ///< Changed Attached Namespace List (Base 2.4, 3.1.3.5, Figure 31, page 48, PDF page 74)
	XNVME_SPEC_BASE_LOG_PAGE_COMMANDS_SUPPORTED_AND_EFFECTS = 0x5, ///< Commands Supported and Effects (Base 2.4, 3.1.3.5, Figure 31, page 48, PDF page 74)
	XNVME_SPEC_BASE_LOG_PAGE_DEVICE_SELF_TEST = 0x6, ///< Device Self-test (Base 2.4, 3.1.3.5, Figure 31, page 48, PDF page 74)
	XNVME_SPEC_BASE_LOG_PAGE_TELEMETRY_HOST_INITIATED = 0x7, ///< Telemetry Host-Initiated (Base 2.4, 3.1.3.5, Figure 31, page 48, PDF page 74)
	XNVME_SPEC_BASE_LOG_PAGE_TELEMETRY_CONTROLLER_INITIATED = 0x8, ///< Telemetry Controller-Initiated (Base 2.4, 3.1.3.5, Figure 31, page 48, PDF page 74)
	XNVME_SPEC_BASE_LOG_PAGE_ENDURANCE_GROUP_INFORMATION = 0x9, ///< Endurance Group Information (Base 2.4, 3.1.3.5, Figure 31, page 48, PDF page 74)
	XNVME_SPEC_BASE_LOG_PAGE_PREDICTABLE_LATENCY_PER_NVM_SET = 0xa, ///< Predictable Latency Per NVM Set (Base 2.4, 3.1.3.5, Figure 31, page 48, PDF page 74)
	XNVME_SPEC_BASE_LOG_PAGE_PREDICTABLE_LATENCY_EVENT_AGGREGATE = 0xb, ///< Predictable Latency Event Aggregate (Base 2.4, 3.1.3.5, Figure 31, page 48, PDF page 74)
	XNVME_SPEC_BASE_LOG_PAGE_ASYMMETRIC_NAMESPACE_ACCESS = 0xc, ///< Asymmetric Namespace Access (Base 2.4, 3.1.3.5, Figure 31, page 48, PDF page 74)
	XNVME_SPEC_BASE_LOG_PAGE_PERSISTENT_EVENT = 0xd, ///< Persistent Event (Base 2.4, 3.1.3.5, Figure 31, page 48, PDF page 74)
	XNVME_SPEC_BASE_LOG_PAGE_PERSISTENT_EVENT_LOG = 0xd, ///< Persistent Event Log (Base 2.4, 5.2.13, Figure 209, page 215, PDF page 241)
	XNVME_SPEC_BASE_LOG_PAGE_LBA_STATUS_INFORMATION = 0xe, ///< LBA Status Information (Base 2.4, 3.1.3.5, Figure 31, page 48, PDF page 74)
	XNVME_SPEC_BASE_LOG_PAGE_ENDURANCE_GROUP_EVENT_AGGREGATE = 0xf, ///< Endurance Group Event Aggregate (Base 2.4, 3.1.3.5, Figure 31, page 48, PDF page 74)
	XNVME_SPEC_BASE_LOG_PAGE_MEDIA_UNIT_STATUS = 0x10, ///< Media Unit Status (Base 2.4, 3.1.3.5, Figure 31, page 48, PDF page 74)
	XNVME_SPEC_BASE_LOG_PAGE_SUPPORTED_CAPACITY_CONFIGURATION_LIST = 0x11, ///< Supported Capacity Configuration List (Base 2.4, 3.1.3.5, Figure 31, page 48, PDF page 74)
	XNVME_SPEC_BASE_LOG_PAGE_FEATURE_IDENTIFIERS_SUPPORTED_AND_EFFECTS = 0x12, ///< Feature Identifiers Supported and Effects (Base 2.4, 3.1.3.5, Figure 31, page 48, PDF page 74)
	XNVME_SPEC_BASE_LOG_PAGE_NVME_MI_COMMANDS_SUPPORTED_AND_EFFECTS = 0x13, ///< NVMe-MI Commands Supported and Effects (Base 2.4, 3.1.3.5, Figure 31, page 48, PDF page 74)
	XNVME_SPEC_BASE_LOG_PAGE_COMMAND_AND_FEATURE_LOCKDOWN = 0x14, ///< Command and Feature Lockdown (Base 2.4, 3.1.3.5, Figure 31, page 48, PDF page 74)
	XNVME_SPEC_BASE_LOG_PAGE_BOOT_PARTITION = 0x15, ///< Boot Partition (Base 2.4, 3.1.3.5, Figure 31, page 48, PDF page 74)
	XNVME_SPEC_BASE_LOG_PAGE_ROTATIONAL_MEDIA_INFORMATION = 0x16, ///< Rotational Media Information (Base 2.4, 3.1.3.5, Figure 31, page 48, PDF page 74)
	XNVME_SPEC_BASE_LOG_PAGE_DISPERSED_NAMESPACE_PARTICIPATING_NVM_SUBSYSTEMS = 0x17, ///< Dispersed Namespace Participating NVM Subsystems (Base 2.4, 3.1.3.5, Figure 31, page 48, PDF page 74)
	XNVME_SPEC_BASE_LOG_PAGE_MANAGEMENT_ADDRESS_LIST = 0x18, ///< Management Address List (Base 2.4, 3.1.3.5, Figure 31, page 48, PDF page 74)
	XNVME_SPEC_BASE_LOG_PAGE_PHYSICAL_INTERFACE_RECEIVER_EYE_OPENING_MEASUREMENT = 0x19, ///< Physical Interface Receiver Eye Opening Measurement (Base 2.4, 3.1.3.5, Figure 31, page 48, PDF page 74)
	XNVME_SPEC_BASE_LOG_PAGE_REACHABILITY_GROUPS = 0x1a, ///< Reachability Groups (Base 2.4, 3.1.3.5, Figure 31, page 48, PDF page 74)
	XNVME_SPEC_BASE_LOG_PAGE_REACHABILITY_ASSOCIATIONS = 0x1b, ///< Reachability Associations (Base 2.4, 3.1.3.5, Figure 31, page 48, PDF page 74)
	XNVME_SPEC_BASE_LOG_PAGE_CHANGED_ALLOCATED_NAMESPACE_LIST = 0x1c, ///< Changed Allocated Namespace List (Base 2.4, 3.1.3.5, Figure 31, page 48, PDF page 74)
	XNVME_SPEC_BASE_LOG_PAGE_DEVICE_PERSONALITIES = 0x1d, ///< Device Personalities (Base 2.4, 3.1.3.5, Figure 31, page 48, PDF page 74)
	XNVME_SPEC_BASE_LOG_PAGE_CROSS_CONTROLLER_RESET = 0x1e, ///< Cross-Controller Reset (Base 2.4, 3.1.3.5, Figure 31, page 48, PDF page 74)
	XNVME_SPEC_BASE_LOG_PAGE_LOST_HOST_COMMUNICATION = 0x1f, ///< Lost Host Communication (Base 2.4, 3.1.3.5, Figure 31, page 48, PDF page 74)
	XNVME_SPEC_BASE_LOG_PAGE_FDP_CONFIGURATIONS = 0x20, ///< FDP Configurations (Base 2.4, 3.1.3.5, Figure 31, page 48, PDF page 74)
	XNVME_SPEC_BASE_LOG_PAGE_RECLAIM_UNIT_HANDLE_USAGE = 0x21, ///< Reclaim Unit Handle Usage (Base 2.4, 3.1.3.5, Figure 31, page 48, PDF page 74)
	XNVME_SPEC_BASE_LOG_PAGE_FDP_STATISTICS = 0x22, ///< FDP Statistics (Base 2.4, 3.1.3.5, Figure 31, page 48, PDF page 74)
	XNVME_SPEC_BASE_LOG_PAGE_FDP_EVENTS = 0x23, ///< FDP Events (Base 2.4, 3.1.3.5, Figure 31, page 48, PDF page 74)
	XNVME_SPEC_BASE_LOG_PAGE_MANUFACTURER_DEFAULT_CONFIGURATION_STATUS = 0x24, ///< Manufacturer Default Configuration Status (Base 2.4, 3.1.3.5, Figure 31, page 48, PDF page 74)
	XNVME_SPEC_BASE_LOG_PAGE_MANUFACTURER_DEFAULT_CONFIGURATION = 0x24, ///< Manufacturer Default Configuration (Base 2.4, 5.2.13, Figure 209, page 215, PDF page 241)
	XNVME_SPEC_BASE_LOG_PAGE_POWER_MEASUREMENT = 0x25, ///< Power Measurement (Base 2.4, 3.1.3.5, Figure 31, page 48, PDF page 74)
	XNVME_SPEC_BASE_LOG_PAGE_VOLTAGE_MEASUREMENT = 0x27, ///< Voltage Measurement (Base 2.4, 3.1.3.5, Figure 31, page 48, PDF page 74)
	XNVME_SPEC_BASE_LOG_PAGE_RATE_LIMITING = 0x28, ///< Rate Limiting (Base 2.4, 3.1.3.5, Figure 31, page 48, PDF page 74)
	XNVME_SPEC_BASE_LOG_PAGE_DISCOVERY = 0x70, ///< Discovery (Base 2.4, 3.1.3.5, Figure 31, page 48, PDF page 74)
	XNVME_SPEC_BASE_LOG_PAGE_HOST_DISCOVERY = 0x71, ///< Host Discovery (Base 2.4, 3.1.3.5, Figure 31, page 48, PDF page 74)
	XNVME_SPEC_BASE_LOG_PAGE_AVE_DISCOVERY = 0x72, ///< AVE Discovery (Base 2.4, 3.1.3.5, Figure 31, page 48, PDF page 74)
	XNVME_SPEC_BASE_LOG_PAGE_PULL_MODEL_DDC_REQUEST = 0x73, ///< Pull Model DDC Request (Base 2.4, 3.1.3.5, Figure 31, page 48, PDF page 74)
	XNVME_SPEC_BASE_LOG_PAGE_SANITIZE_NAMESPACE_STATUS_LIST = 0x7f, ///< Sanitize Namespace Status List (Base 2.4, 3.1.3.5, Figure 31, page 48, PDF page 74)
	XNVME_SPEC_BASE_LOG_PAGE_RESERVATION_NOTIFICATION = 0x80, ///< Reservation Notification (Base 2.4, 3.1.3.5, Figure 31, page 48, PDF page 74)
	XNVME_SPEC_BASE_LOG_PAGE_SANITIZE_STATUS = 0x81, ///< Sanitize Status (Base 2.4, 3.1.3.5, Figure 31, page 48, PDF page 74)
	XNVME_SPEC_BASE_LOG_PAGE_PROGRAM_LIST = 0x82, ///< Program List (Base 2.4, 3.1.3.5, Figure 31, page 48, PDF page 74)
	XNVME_SPEC_BASE_LOG_PAGE_DOWNLOADABLE_PROGRAM_TYPES_LIST = 0x83, ///< Downloadable Program Types List (Base 2.4, 3.1.3.5, Figure 31, page 48, PDF page 74)
	XNVME_SPEC_BASE_LOG_PAGE_MEMORY_RANGE_SET_LIST = 0x84, ///< Memory Range Set List (Base 2.4, 3.1.3.5, Figure 31, page 48, PDF page 74)
	XNVME_SPEC_BASE_LOG_PAGE_CHANGED_ZONE_LIST = 0xbf, ///< Changed Zone List (Base 2.4, 3.1.3.5, Figure 31, page 48, PDF page 74)
};

/**
 * The types of status
 */
enum xnvme_spec_base_status_code_type {
	XNVME_SPEC_BASE_STATUS_CODE_TYPE_GENERIC_COMMAND_STATUS = 0x0, ///< Generic Command Status (Base 2.4, 4.2.3, Figure 102, page 146, PDF page 172)
	XNVME_SPEC_BASE_STATUS_CODE_TYPE_COMMAND_SPECIFIC_STATUS = 0x1, ///< Command Specific Status (Base 2.4, 4.2.3, Figure 102, page 146, PDF page 172)
	XNVME_SPEC_BASE_STATUS_CODE_TYPE_MEDIA_AND_DATA_INTEGRITY_ERRORS = 0x2, ///< Media and Data Integrity Errors (Base 2.4, 4.2.3, Figure 102, page 146, PDF page 172)
	XNVME_SPEC_BASE_STATUS_CODE_TYPE_PATH_RELATED_STATUS = 0x3, ///< Path Related Status (Base 2.4, 4.2.3, Figure 102, page 146, PDF page 172)
	XNVME_SPEC_BASE_STATUS_CODE_TYPE_VENDOR_SPECIFIC = 0x7, ///< Vendor Specific (Base 2.4, 4.2.3, Figure 102, page 146, PDF page 172)
};

/**
 * The status codes of the type Generic Command
 */
enum xnvme_spec_base_generic_command_status {
	XNVME_SPEC_BASE_GENERIC_COMMAND_STATUS_SUCCESSFUL_COMPLETION = 0x0, ///< Successful Completion (Base 2.4, 4.2.3.1, Figure 103, page 147, PDF page 173)
	XNVME_SPEC_BASE_GENERIC_COMMAND_STATUS_INVALID_COMMAND_OPCODE = 0x1, ///< Invalid Command Opcode (Base 2.4, 4.2.3.1, Figure 103, page 147, PDF page 173)
	XNVME_SPEC_BASE_GENERIC_COMMAND_STATUS_INVALID_FIELD_IN_COMMAND = 0x2, ///< Invalid Field in Command (Base 2.4, 4.2.3.1, Figure 103, page 147, PDF page 173)
	XNVME_SPEC_BASE_GENERIC_COMMAND_STATUS_COMMAND_ID_CONFLICT = 0x3, ///< Command ID Conflict (Base 2.4, 4.2.3.1, Figure 103, page 147, PDF page 173)
	XNVME_SPEC_BASE_GENERIC_COMMAND_STATUS_DATA_TRANSFER_ERROR = 0x4, ///< Data Transfer Error (Base 2.4, 4.2.3.1, Figure 103, page 147, PDF page 173)
	XNVME_SPEC_BASE_GENERIC_COMMAND_STATUS_COMMANDS_ABORTED_DUE_TO_POWER_LOSS_NOTIFICATION = 0x5, ///< Commands Aborted due to Power Loss Notification (Base 2.4, 4.2.3.1, Figure 103, page 147, PDF page 173)
	XNVME_SPEC_BASE_GENERIC_COMMAND_STATUS_INTERNAL_ERROR = 0x6, ///< Internal Error (Base 2.4, 4.2.3.1, Figure 103, page 147, PDF page 173)
	XNVME_SPEC_BASE_GENERIC_COMMAND_STATUS_COMMAND_ABORT_REQUESTED = 0x7, ///< Command Abort Requested (Base 2.4, 4.2.3.1, Figure 103, page 147, PDF page 173)
	XNVME_SPEC_BASE_GENERIC_COMMAND_STATUS_COMMAND_ABORTED_DUE_TO_SQ_DELETION = 0x8, ///< Command Aborted due to SQ Deletion (Base 2.4, 4.2.3.1, Figure 103, page 147, PDF page 173)
	XNVME_SPEC_BASE_GENERIC_COMMAND_STATUS_COMMAND_ABORTED_DUE_TO_FAILED_FUSED_COMMAND = 0x9, ///< Command Aborted due to Failed Fused Command (Base 2.4, 4.2.3.1, Figure 103, page 147, PDF page 173)
	XNVME_SPEC_BASE_GENERIC_COMMAND_STATUS_COMMAND_ABORTED_DUE_TO_MISSING_FUSED_COMMAND = 0xa, ///< Command Aborted due to Missing Fused Command (Base 2.4, 4.2.3.1, Figure 103, page 147, PDF page 173)
	XNVME_SPEC_BASE_GENERIC_COMMAND_STATUS_INVALID_NAMESPACE_OR_FORMAT = 0xb, ///< Invalid Namespace or Format (Base 2.4, 4.2.3.1, Figure 103, page 147, PDF page 173)
	XNVME_SPEC_BASE_GENERIC_COMMAND_STATUS_COMMAND_SEQUENCE_ERROR = 0xc, ///< Command Sequence Error (Base 2.4, 4.2.3.1, Figure 103, page 147, PDF page 173)
	XNVME_SPEC_BASE_GENERIC_COMMAND_STATUS_INVALID_SGL_SEGMENT_DESCRIPTOR = 0xd, ///< Invalid SGL Segment Descriptor (Base 2.4, 4.2.3.1, Figure 103, page 147, PDF page 173)
	XNVME_SPEC_BASE_GENERIC_COMMAND_STATUS_INVALID_NUMBER_OF_SGL_DESCRIPTORS = 0xe, ///< Invalid Number of SGL Descriptors (Base 2.4, 4.2.3.1, Figure 103, page 147, PDF page 173)
	XNVME_SPEC_BASE_GENERIC_COMMAND_STATUS_DATA_SGL_LENGTH_INVALID = 0xf, ///< Data SGL Length Invalid (Base 2.4, 4.2.3.1, Figure 103, page 147, PDF page 173)
	XNVME_SPEC_BASE_GENERIC_COMMAND_STATUS_METADATA_SGL_LENGTH_INVALID = 0x10, ///< Metadata SGL Length Invalid (Base 2.4, 4.2.3.1, Figure 103, page 147, PDF page 173)
	XNVME_SPEC_BASE_GENERIC_COMMAND_STATUS_SGL_DESCRIPTOR_TYPE_INVALID = 0x11, ///< SGL Descriptor Type Invalid (Base 2.4, 4.2.3.1, Figure 103, page 147, PDF page 173)
	XNVME_SPEC_BASE_GENERIC_COMMAND_STATUS_INVALID_USE_OF_CONTROLLER_MEMORY_BUFFER = 0x12, ///< Invalid Use of Controller Memory Buffer (Base 2.4, 4.2.3.1, Figure 103, page 147, PDF page 173)
	XNVME_SPEC_BASE_GENERIC_COMMAND_STATUS_PRP_OFFSET_INVALID = 0x13, ///< PRP Offset Invalid (Base 2.4, 4.2.3.1, Figure 103, page 147, PDF page 173)
	XNVME_SPEC_BASE_GENERIC_COMMAND_STATUS_ATOMIC_WRITE_UNIT_EXCEEDED = 0x14, ///< Atomic Write Unit Exceeded (Base 2.4, 4.2.3.1, Figure 103, page 147, PDF page 173)
	XNVME_SPEC_BASE_GENERIC_COMMAND_STATUS_OPERATION_DENIED = 0x15, ///< Operation Denied (Base 2.4, 4.2.3.1, Figure 103, page 147, PDF page 173)
	XNVME_SPEC_BASE_GENERIC_COMMAND_STATUS_SGL_OFFSET_INVALID = 0x16, ///< SGL Offset Invalid (Base 2.4, 4.2.3.1, Figure 103, page 147, PDF page 173)
	XNVME_SPEC_BASE_GENERIC_COMMAND_STATUS_HOST_IDENTIFIER_INCONSISTENT_FORMAT = 0x18, ///< Host Identifier Inconsistent Format (Base 2.4, 4.2.3.1, Figure 103, page 147, PDF page 173)
	XNVME_SPEC_BASE_GENERIC_COMMAND_STATUS_KEEP_ALIVE_TIMER_EXPIRED = 0x19, ///< Keep Alive Timer Expired (Base 2.4, 4.2.3.1, Figure 103, page 147, PDF page 173)
	XNVME_SPEC_BASE_GENERIC_COMMAND_STATUS_KEEP_ALIVE_TIMEOUT_INVALID = 0x1a, ///< Keep Alive Timeout Invalid (Base 2.4, 4.2.3.1, Figure 103, page 147, PDF page 173)
	XNVME_SPEC_BASE_GENERIC_COMMAND_STATUS_COMMAND_ABORTED_DUE_TO_PREEMPT_AND_ABORT = 0x1b, ///< Command Aborted due to Preempt and Abort (Base 2.4, 4.2.3.1, Figure 103, page 147, PDF page 173)
	XNVME_SPEC_BASE_GENERIC_COMMAND_STATUS_SANITIZE_FAILED = 0x1c, ///< Sanitize Failed (Base 2.4, 4.2.3.1, Figure 103, page 147, PDF page 173)
	XNVME_SPEC_BASE_GENERIC_COMMAND_STATUS_SANITIZE_IN_PROGRESS = 0x1d, ///< Sanitize In Progress (Base 2.4, 4.2.3.1, Figure 103, page 147, PDF page 173)
	XNVME_SPEC_BASE_GENERIC_COMMAND_STATUS_SGL_DATA_BLOCK_GRANULARITY_INVALID = 0x1e, ///< SGL Data Block Granularity Invalid (Base 2.4, 4.2.3.1, Figure 103, page 147, PDF page 173)
	XNVME_SPEC_BASE_GENERIC_COMMAND_STATUS_COMMAND_NOT_SUPPORTED_FOR_QUEUE_IN_CMB = 0x1f, ///< Command Not Supported for Queue in CMB (Base 2.4, 4.2.3.1, Figure 103, page 147, PDF page 173)
	XNVME_SPEC_BASE_GENERIC_COMMAND_STATUS_NAMESPACE_IS_WRITE_PROTECTED = 0x20, ///< Namespace is Write Protected (Base 2.4, 4.2.3.1, Figure 103, page 147, PDF page 173)
	XNVME_SPEC_BASE_GENERIC_COMMAND_STATUS_COMMAND_INTERRUPTED = 0x21, ///< Command Interrupted (Base 2.4, 4.2.3.1, Figure 103, page 147, PDF page 173)
	XNVME_SPEC_BASE_GENERIC_COMMAND_STATUS_TRANSIENT_TRANSPORT_ERROR = 0x22, ///< Transient Transport Error (Base 2.4, 4.2.3.1, Figure 103, page 147, PDF page 173)
	XNVME_SPEC_BASE_GENERIC_COMMAND_STATUS_COMMAND_PROHIBITED_BY_COMMAND_AND_FEATURE_LOCKDOWN = 0x23, ///< Command Prohibited by Command and Feature Lockdown (Base 2.4, 4.2.3.1, Figure 103, page 147, PDF page 173)
	XNVME_SPEC_BASE_GENERIC_COMMAND_STATUS_ADMIN_COMMAND_MEDIA_NOT_READY = 0x24, ///< Admin Command Media Not Ready (Base 2.4, 4.2.3.1, Figure 103, page 147, PDF page 173)
	XNVME_SPEC_BASE_GENERIC_COMMAND_STATUS_INVALID_KEY_TAG = 0x25, ///< Invalid Key Tag (Base 2.4, 4.2.3.1, Figure 103, page 147, PDF page 173)
	XNVME_SPEC_BASE_GENERIC_COMMAND_STATUS_HOST_DISPERSED_NAMESPACE_SUPPORT_NOT_ENABLED = 0x26, ///< Host Dispersed Namespace Support Not Enabled (Base 2.4, 4.2.3.1, Figure 103, page 147, PDF page 173)
	XNVME_SPEC_BASE_GENERIC_COMMAND_STATUS_HOST_IDENTIFIER_NOT_INITIALIZED = 0x27, ///< Host Identifier Not Initialized (Base 2.4, 4.2.3.1, Figure 103, page 147, PDF page 173)
	XNVME_SPEC_BASE_GENERIC_COMMAND_STATUS_INCORRECT_KEY = 0x28, ///< Incorrect Key (Base 2.4, 4.2.3.1, Figure 103, page 147, PDF page 173)
	XNVME_SPEC_BASE_GENERIC_COMMAND_STATUS_FDP_DISABLED = 0x29, ///< FDP Disabled (Base 2.4, 4.2.3.1, Figure 103, page 147, PDF page 173)
	XNVME_SPEC_BASE_GENERIC_COMMAND_STATUS_INVALID_PLACEMENT_HANDLE_LIST = 0x2a, ///< Invalid Placement Handle List (Base 2.4, 4.2.3.1, Figure 103, page 147, PDF page 173)
	XNVME_SPEC_BASE_GENERIC_COMMAND_STATUS_SANITIZE_NAMESPACE_FAILED = 0x2b, ///< Sanitize Namespace Failed (Base 2.4, 4.2.3.1, Figure 103, page 147, PDF page 173)
	XNVME_SPEC_BASE_GENERIC_COMMAND_STATUS_SANITIZE_NAMESPACE_IN_PROGRESS = 0x2c, ///< Sanitize Namespace In Progress (Base 2.4, 4.2.3.1, Figure 103, page 147, PDF page 173)
	XNVME_SPEC_BASE_GENERIC_COMMAND_STATUS_FAILED_TO_RESTORE_CONFIGURATION = 0x2d, ///< Failed to Restore Configuration (Base 2.4, 4.2.3.1, Figure 103, page 147, PDF page 173)
	XNVME_SPEC_BASE_GENERIC_COMMAND_STATUS_LBA_OUT_OF_RANGE = 0x80, ///< LBA Out of Range (Base 2.4, 4.2.3.1, Figure 103, page 147, PDF page 173)
	XNVME_SPEC_BASE_GENERIC_COMMAND_STATUS_CAPACITY_EXCEEDED = 0x81, ///< Capacity Exceeded (Base 2.4, 4.2.3.1, Figure 103, page 147, PDF page 173)
	XNVME_SPEC_BASE_GENERIC_COMMAND_STATUS_NAMESPACE_NOT_READY = 0x82, ///< Namespace Not Ready (Base 2.4, 4.2.3.1, Figure 103, page 147, PDF page 173)
	XNVME_SPEC_BASE_GENERIC_COMMAND_STATUS_RESERVATION_CONFLICT = 0x83, ///< Reservation Conflict (Base 2.4, 4.2.3.1, Figure 103, page 147, PDF page 173)
	XNVME_SPEC_BASE_GENERIC_COMMAND_STATUS_FORMAT_IN_PROGRESS = 0x84, ///< Format In Progress (Base 2.4, 4.2.3.1, Figure 103, page 147, PDF page 173)
	XNVME_SPEC_BASE_GENERIC_COMMAND_STATUS_INVALID_VALUE_SIZE = 0x85, ///< Invalid Value Size (Base 2.4, 4.2.3.1, Figure 103, page 147, PDF page 173)
	XNVME_SPEC_BASE_GENERIC_COMMAND_STATUS_INVALID_KEY_SIZE = 0x86, ///< Invalid Key Size (Base 2.4, 4.2.3.1, Figure 103, page 147, PDF page 173)
	XNVME_SPEC_BASE_GENERIC_COMMAND_STATUS_KV_KEY_DOES_NOT_EXIST = 0x87, ///< KV Key Does Not Exist (Base 2.4, 4.2.3.1, Figure 103, page 147, PDF page 173)
	XNVME_SPEC_BASE_GENERIC_COMMAND_STATUS_UNRECOVERED_ERROR = 0x88, ///< Unrecovered Error (Base 2.4, 4.2.3.1, Figure 103, page 147, PDF page 173)
	XNVME_SPEC_BASE_GENERIC_COMMAND_STATUS_KEY_EXISTS = 0x89, ///< Key Exists (Base 2.4, 4.2.3.1, Figure 103, page 147, PDF page 173)
};

/**
 * The status codes of the type Command Specific
 */
enum xnvme_spec_base_command_specific_status {
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_COMPLETION_QUEUE_INVALID = 0x0, ///< Completion Queue Invalid (Base 2.4, 4.2.3.2, Figure 104, page 151, PDF page 177)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_INVALID_QUEUE_IDENTIFIER = 0x1, ///< Invalid Queue Identifier (Base 2.4, 4.2.3.2, Figure 104, page 151, PDF page 177)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_INVALID_QUEUE_SIZE = 0x2, ///< Invalid Queue Size (Base 2.4, 4.2.3.2, Figure 104, page 151, PDF page 177)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_ABORT_COMMAND_LIMIT_EXCEEDED = 0x3, ///< Abort Command Limit Exceeded (Base 2.4, 4.2.3.2, Figure 104, page 151, PDF page 177)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_ASYNCHRONOUS_EVENT_REQUEST_LIMIT = 0x5, ///< Asynchronous Event Request Limit (Base 2.4, 4.2.3.2, Figure 104, page 151, PDF page 177)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_INVALID_FIRMWARE_SLOT = 0x6, ///< Invalid Firmware Slot (Base 2.4, 4.2.3.2, Figure 104, page 151, PDF page 177)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_INVALID_FIRMWARE_IMAGE = 0x7, ///< Invalid Firmware Image (Base 2.4, 4.2.3.2, Figure 104, page 151, PDF page 177)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_INVALID_INTERRUPT_VECTOR = 0x8, ///< Invalid Interrupt Vector (Base 2.4, 4.2.3.2, Figure 104, page 151, PDF page 177)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_INVALID_LOG_PAGE = 0x9, ///< Invalid Log Page (Base 2.4, 4.2.3.2, Figure 104, page 151, PDF page 177)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_INVALID_FORMAT = 0xa, ///< Invalid Format (Base 2.4, 4.2.3.2, Figure 104, page 151, PDF page 177)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_FIRMWARE_ACTIVATION_REQUIRES_CONVENTIONAL = 0xb, ///< Firmware Activation Requires Conventional (Base 2.4, 4.2.3.2, Figure 104, page 151, PDF page 177)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_INVALID_QUEUE_DELETION = 0xc, ///< Invalid Queue Deletion (Base 2.4, 4.2.3.2, Figure 104, page 151, PDF page 177)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_FEATURE_IDENTIFIER_NOT_SAVEABLE = 0xd, ///< Feature Identifier Not Saveable (Base 2.4, 4.2.3.2, Figure 104, page 151, PDF page 177)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_FEATURE_NOT_CHANGEABLE = 0xe, ///< Feature Not Changeable (Base 2.4, 4.2.3.2, Figure 104, page 151, PDF page 177)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_FEATURE_NOT_NAMESPACE_SPECIFIC = 0xf, ///< Feature Not Namespace Specific (Base 2.4, 4.2.3.2, Figure 104, page 151, PDF page 177)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_FIRMWARE_ACTIVATION_REQUIRES_NVM = 0x10, ///< Firmware Activation Requires NVM (Base 2.4, 4.2.3.2, Figure 104, page 151, PDF page 177)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_FIRMWARE_ACTIVATION_REQUIRES_CONTROLLER = 0x11, ///< Firmware Activation Requires Controller (Base 2.4, 4.2.3.2, Figure 104, page 151, PDF page 177)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_FIRMWARE_ACTIVATION_REQUIRES_MAXIMUM = 0x12, ///< Firmware Activation Requires Maximum (Base 2.4, 4.2.3.2, Figure 104, page 151, PDF page 177)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_FIRMWARE_ACTIVATION_PROHIBITED = 0x13, ///< Firmware Activation Prohibited (Base 2.4, 4.2.3.2, Figure 104, page 151, PDF page 177)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_OVERLAPPING_RANGE = 0x14, ///< Overlapping Range (Base 2.4, 4.2.3.2, Figure 104, page 151, PDF page 177)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_NAMESPACE_INSUFFICIENT_CAPACITY = 0x15, ///< Namespace Insufficient Capacity (Base 2.4, 4.2.3.2, Figure 104, page 151, PDF page 177)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_NAMESPACE_IDENTIFIER_UNAVAILABLE = 0x16, ///< Namespace Identifier Unavailable (Base 2.4, 4.2.3.2, Figure 104, page 151, PDF page 177)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_NAMESPACE_ALREADY_ATTACHED = 0x18, ///< Namespace Already Attached (Base 2.4, 4.2.3.2, Figure 104, page 151, PDF page 177)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_NAMESPACE_IS_PRIVATE = 0x19, ///< Namespace Is Private (Base 2.4, 4.2.3.2, Figure 104, page 151, PDF page 177)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_NAMESPACE_NOT_ATTACHED = 0x1a, ///< Namespace Not Attached (Base 2.4, 4.2.3.2, Figure 104, page 151, PDF page 177)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_THIN_PROVISIONING_NOT_SUPPORTED = 0x1b, ///< Thin Provisioning Not Supported (Base 2.4, 4.2.3.2, Figure 104, page 151, PDF page 177)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_CONTROLLER_LIST_INVALID = 0x1c, ///< Controller List Invalid (Base 2.4, 4.2.3.2, Figure 104, page 151, PDF page 177)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_DEVICE_SELF_TEST_IN_PROGRESS = 0x1d, ///< Device Self-test In Progress (Base 2.4, 4.2.3.2, Figure 104, page 151, PDF page 177)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_BOOT_PARTITION_WRITE_PROHIBITED = 0x1e, ///< Boot Partition Write Prohibited (Base 2.4, 4.2.3.2, Figure 104, page 151, PDF page 177)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_INVALID_CONTROLLER_IDENTIFIER = 0x1f, ///< Invalid Controller Identifier (Base 2.4, 4.2.3.2, Figure 104, page 151, PDF page 177)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_INVALID_SECONDARY_CONTROLLER_STATE = 0x20, ///< Invalid Secondary Controller State (Base 2.4, 4.2.3.2, Figure 104, page 151, PDF page 177)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_INVALID_RESOURCE_IDENTIFIER = 0x22, ///< Invalid Resource Identifier (Base 2.4, 4.2.3.2, Figure 104, page 151, PDF page 177)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_ANA_GROUP_IDENTIFIER_INVALID = 0x24, ///< ANA Group Identifier Invalid (Base 2.4, 4.2.3.2, Figure 104, page 151, PDF page 177)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_ANA_ATTACH_FAILED = 0x25, ///< ANA Attach Failed (Base 2.4, 4.2.3.2, Figure 104, page 151, PDF page 177)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_INSUFFICIENT_CAPACITY = 0x26, ///< Insufficient Capacity (Base 2.4, 4.2.3.2, Figure 104, page 151, PDF page 177)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_NAMESPACE_ATTACHMENT_LIMIT_EXCEEDED = 0x27, ///< Namespace Attachment Limit Exceeded (Base 2.4, 4.2.3.2, Figure 104, page 151, PDF page 177)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_INVALID_IO_COMMAND_SET = 0x2c, ///< Invalid I/O Command Set (Base 2.4, 4.2.3.2, Figure 104, page 151, PDF page 177)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_IDENTIFIER_UNAVAILABLE = 0x2d, ///< Identifier Unavailable (Base 2.4, 4.2.3.2, Figure 104, page 151, PDF page 177)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_NAMESPACE_IS_DISPERSED = 0x2e, ///< Namespace Is Dispersed (Base 2.4, 4.2.3.2, Figure 104, page 151, PDF page 177)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_INVALID_DISCOVERY_INFORMATION = 0x2f, ///< Invalid Discovery Information (Base 2.4, 4.2.3.2, Figure 104, page 151, PDF page 177)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_ZONING_DATA_STRUCTURE_LOCKED = 0x30, ///< Zoning Data Structure Locked (Base 2.4, 4.2.3.2, Figure 104, page 151, PDF page 177)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_INSUFFICIENT_DISCOVERY_RESOURCES = 0x32, ///< Insufficient Discovery Resources (Base 2.4, 4.2.3.2, Figure 104, page 151, PDF page 177)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_REQUESTED_FUNCTION_DISABLED = 0x33, ///< Requested Function Disabled (Base 2.4, 4.2.3.2, Figure 104, page 151, PDF page 177)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_ZONEGROUP_ORIGINATOR_INVALID = 0x34, ///< ZoneGroup Originator Invalid (Base 2.4, 4.2.3.2, Figure 104, page 151, PDF page 177)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_INVALID_HOST = 0x35, ///< Invalid Host (Base 2.4, 4.2.3.2, Figure 104, page 151, PDF page 177)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_INVALID_NVM_SUBSYSTEM = 0x36, ///< Invalid NVM Subsystem (Base 2.4, 4.2.3.2, Figure 104, page 151, PDF page 177)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_INVALID_CONTROLLER_DATA_QUEUE = 0x37, ///< Invalid Controller Data Queue (Base 2.4, 4.2.3.2, Figure 104, page 151, PDF page 177)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_NOT_ENOUGH_RESOURCES = 0x38, ///< Not Enough Resources (Base 2.4, 4.2.3.2, Figure 104, page 151, PDF page 177)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_CONTROLLER_SUSPENDED = 0x39, ///< Controller Suspended (Base 2.4, 4.2.3.2, Figure 104, page 151, PDF page 177)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_CONTROLLER_NOT_SUSPENDED = 0x3a, ///< Controller Not Suspended (Base 2.4, 4.2.3.2, Figure 104, page 151, PDF page 177)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_CONTROLLER_DATA_QUEUE_FULL = 0x3b, ///< Controller Data Queue Full (Base 2.4, 4.2.3.2, Figure 104, page 151, PDF page 177)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_REQUEST_EXCEEDS_MAXIMUM_NAMESPACE = 0x3c, ///< Request Exceeds Maximum Namespace (Base 2.4, 4.2.3.2, Figure 104, page 151, PDF page 177)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_MANUFACTURING_DEFAULT_PERSONALITY_REQUIRED = 0x3d, ///< Manufacturing Default Personality Required (Base 2.4, 4.2.3.2, Figure 104, page 151, PDF page 177)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_INVALID_POWER_LIMIT = 0x3e, ///< Invalid Power Limit (Base 2.4, 4.2.3.2, Figure 104, page 151, PDF page 177)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_CROSS_CONTROLLER_RESET_IN_PROGRESS = 0x3f, ///< Cross-Controller Reset in Progress (Base 2.4, 4.2.3.2, Figure 104, page 151, PDF page 177)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_CROSS_CONTROLLER_RESET_LIMIT_EXCEEDED = 0x41, ///< Cross-Controller Reset Limit Exceeded (Base 2.4, 4.2.3.2, Figure 104, page 151, PDF page 177)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_CONTROLLER_ACTIVE = 0x42, ///< Controller Active (Base 2.4, 4.2.3.2, Figure 104, page 151, PDF page 177)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_INVALID_EXPORTED_ASSOCIATION = 0x43, ///< Invalid Exported Association (Base 2.4, 4.2.3.2, Figure 104, page 151, PDF page 177)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_INVALID_EXPORTED_CONFIGURATION_STATE = 0x44, ///< Invalid Exported Configuration State (Base 2.4, 4.2.3.2, Figure 104, page 151, PDF page 177)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_CONFLICTING_ATTRIBUTES = 0x80, ///< Conflicting Attributes (Base 2.4, 4.2.3.2, Figure 105, page 152, PDF page 178)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_INVALID_PROTECTION_INFORMATION = 0x81, ///< Invalid Protection Information (Base 2.4, 4.2.3.2, Figure 105, page 152, PDF page 178)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_COMMAND_SIZE_LIMIT_EXCEEDED = 0x83, ///< Command Size Limit Exceeded (Base 2.4, 4.2.3.2, Figure 105, page 152, PDF page 178)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_INVALID_COMMAND_ID = 0x84, ///< Invalid Command ID (Base 2.4, 4.2.3.2, Figure 105, page 152, PDF page 178)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_INCOMPATIBLE_NAMESPACE_OR_FORMAT = 0x85, ///< Incompatible Namespace or Format (Base 2.4, 4.2.3.2, Figure 105, page 152, PDF page 178)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_FAST_COPY_NOT_POSSIBLE = 0x86, ///< Fast Copy Not Possible (Base 2.4, 4.2.3.2, Figure 105, page 152, PDF page 178)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_OVERLAPPING_IO_RANGE = 0x87, ///< Overlapping I/O Range (Base 2.4, 4.2.3.2, Figure 105, page 152, PDF page 178)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_NAMESPACE_NOT_REACHABLE = 0x88, ///< Namespace Not Reachable (Base 2.4, 4.2.3.2, Figure 105, page 152, PDF page 178)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_INSUFFICIENT_RESOURCES = 0x89, ///< Insufficient Resources (Base 2.4, 4.2.3.2, Figure 105, page 152, PDF page 178)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_INSUFFICIENT_PROGRAM_RESOURCES = 0x8a, ///< Insufficient Program Resources (Base 2.4, 4.2.3.2, Figure 105, page 152, PDF page 178)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_INVALID_MEMORY_NAMESPACE = 0x8b, ///< Invalid Memory Namespace (Base 2.4, 4.2.3.2, Figure 105, page 152, PDF page 178)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_INVALID_MEMORY_RANGE_SET = 0x8c, ///< Invalid Memory Range Set (Base 2.4, 4.2.3.2, Figure 105, page 152, PDF page 178)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_INVALID_PROGRAM_DATA = 0x8e, ///< Invalid Program Data (Base 2.4, 4.2.3.2, Figure 105, page 152, PDF page 178)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_INVALID_PROGRAM_INDEX = 0x8f, ///< Invalid Program Index (Base 2.4, 4.2.3.2, Figure 105, page 152, PDF page 178)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_INVALID_PROGRAM_TYPE = 0x90, ///< Invalid Program Type (Base 2.4, 4.2.3.2, Figure 105, page 152, PDF page 178)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_MAXIMUM_MEMORY_RANGES_EXCEEDED = 0x91, ///< Maximum Memory Ranges Exceeded (Base 2.4, 4.2.3.2, Figure 105, page 152, PDF page 178)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_MAXIMUM_PROGRAMS_ACTIVATED = 0x93, ///< Maximum Programs Activated (Base 2.4, 4.2.3.2, Figure 105, page 152, PDF page 178)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_MAXIMUM_PROGRAM_BYTES_EXCEEDED = 0x94, ///< Maximum Program Bytes Exceeded (Base 2.4, 4.2.3.2, Figure 105, page 152, PDF page 178)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_NO_PROGRAM = 0x96, ///< No Program (Base 2.4, 4.2.3.2, Figure 105, page 152, PDF page 178)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_OVERLAPPING_MEMORY_RANGES = 0x97, ///< Overlapping Memory Ranges (Base 2.4, 4.2.3.2, Figure 105, page 152, PDF page 178)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_PROGRAM_NOT_ACTIVATED = 0x98, ///< Program Not Activated (Base 2.4, 4.2.3.2, Figure 105, page 152, PDF page 178)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_PROGRAM_IN_USE = 0x99, ///< Program In Use (Base 2.4, 4.2.3.2, Figure 105, page 152, PDF page 178)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_PROGRAM_INDEX_NOT_DOWNLOADABLE = 0x9a, ///< Program Index Not Downloadable (Base 2.4, 4.2.3.2, Figure 105, page 152, PDF page 178)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_PROGRAM_TOO_BIG = 0x9b, ///< Program Too Big (Base 2.4, 4.2.3.2, Figure 105, page 152, PDF page 178)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_SUCCESSFUL_MEDIA_VERIFICATION_READ = 0x9c, ///< Successful Media Verification Read (Base 2.4, 4.2.3.2, Figure 105, page 152, PDF page 178)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_ZONED_BOUNDARY_ERROR = 0xb8, ///< Zoned Boundary Error (Base 2.4, 4.2.3.2, Figure 105, page 152, PDF page 178)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_ZONE_IS_FULL = 0xb9, ///< Zone Is Full (Base 2.4, 4.2.3.2, Figure 105, page 152, PDF page 178)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_ZONE_IS_READ_ONLY = 0xba, ///< Zone Is Read Only (Base 2.4, 4.2.3.2, Figure 105, page 152, PDF page 178)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_ZONE_IS_OFFLINE = 0xbb, ///< Zone Is Offline (Base 2.4, 4.2.3.2, Figure 105, page 152, PDF page 178)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_ZONE_INVALID_WRITE = 0xbc, ///< Zone Invalid Write (Base 2.4, 4.2.3.2, Figure 105, page 152, PDF page 178)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_TOO_MANY_ACTIVE_ZONES = 0xbd, ///< Too Many Active Zones (Base 2.4, 4.2.3.2, Figure 105, page 152, PDF page 178)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_TOO_MANY_OPEN_ZONES = 0xbe, ///< Too Many Open Zones (Base 2.4, 4.2.3.2, Figure 105, page 152, PDF page 178)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_INVALID_ZONE_STATE_TRANSITION = 0xbf, ///< Invalid Zone State Transition (Base 2.4, 4.2.3.2, Figure 105, page 152, PDF page 178)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_STATUS_ASYNCHRONOUS_EVENT_REQUEST_LIMIT_EXCEEDED = 0x5, ///< Asynchronous Event Request Limit Exceeded (Base 2.4, 5.2.2.1, Figure 150, page 184, PDF page 210)
};

/**
 * The status codes of the type Command Specific, of Fabrics Command Specific
 */
enum xnvme_spec_base_command_specific_fabrics_command_specific_status {
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_FABRICS_COMMAND_SPECIFIC_STATUS_INCOMPATIBLE_FORMAT = 0x80, ///< Incompatible Format (Base 2.4, 4.2.3.2, Figure 106, page 153, PDF page 179)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_FABRICS_COMMAND_SPECIFIC_STATUS_CONTROLLER_BUSY = 0x81, ///< Controller Busy (Base 2.4, 4.2.3.2, Figure 106, page 153, PDF page 179)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_FABRICS_COMMAND_SPECIFIC_STATUS_CONNECT_INVALID_PARAMETERS = 0x82, ///< Connect Invalid Parameters (Base 2.4, 4.2.3.2, Figure 106, page 153, PDF page 179)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_FABRICS_COMMAND_SPECIFIC_STATUS_CONNECT_RESTART_DISCOVERY = 0x83, ///< Connect Restart Discovery (Base 2.4, 4.2.3.2, Figure 106, page 153, PDF page 179)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_FABRICS_COMMAND_SPECIFIC_STATUS_CONNECT_INVALID_HOST = 0x84, ///< Connect Invalid Host (Base 2.4, 4.2.3.2, Figure 106, page 153, PDF page 179)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_FABRICS_COMMAND_SPECIFIC_STATUS_INVALID_QUEUE_TYPE = 0x85, ///< Invalid Queue Type (Base 2.4, 4.2.3.2, Figure 106, page 153, PDF page 179)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_FABRICS_COMMAND_SPECIFIC_STATUS_DISCOVER_RESTART = 0x90, ///< Discover Restart (Base 2.4, 4.2.3.2, Figure 106, page 153, PDF page 179)
	XNVME_SPEC_BASE_COMMAND_SPECIFIC_FABRICS_COMMAND_SPECIFIC_STATUS_AUTHENTICATION_REQUIRED = 0x91, ///< Authentication Required (Base 2.4, 4.2.3.2, Figure 106, page 153, PDF page 179)
};

/**
 * The status codes of the type Media and Data Integrity Error
 */
enum xnvme_spec_base_media_and_data_integrity_error_status {
	XNVME_SPEC_BASE_MEDIA_AND_DATA_INTEGRITY_ERROR_STATUS_WRITE_FAULT = 0x80, ///< Write Fault (Base 2.4, 4.2.3.3, Figure 107, page 154, PDF page 180)
	XNVME_SPEC_BASE_MEDIA_AND_DATA_INTEGRITY_ERROR_STATUS_UNRECOVERED_READ_ERROR = 0x81, ///< Unrecovered Read Error (Base 2.4, 4.2.3.3, Figure 107, page 154, PDF page 180)
	XNVME_SPEC_BASE_MEDIA_AND_DATA_INTEGRITY_ERROR_STATUS_END_TO_END_GUARD_CHECK_ERROR = 0x82, ///< End-to-end Guard Check Error (Base 2.4, 4.2.3.3, Figure 107, page 154, PDF page 180)
	XNVME_SPEC_BASE_MEDIA_AND_DATA_INTEGRITY_ERROR_STATUS_END_TO_END_APPLICATION_TAG_CHECK_ERROR = 0x83, ///< End-to-end Application Tag Check Error (Base 2.4, 4.2.3.3, Figure 107, page 154, PDF page 180)
	XNVME_SPEC_BASE_MEDIA_AND_DATA_INTEGRITY_ERROR_STATUS_END_TO_END_REFERENCE_TAG_CHECK_ERROR = 0x84, ///< End-to-end Reference Tag Check Error (Base 2.4, 4.2.3.3, Figure 107, page 154, PDF page 180)
	XNVME_SPEC_BASE_MEDIA_AND_DATA_INTEGRITY_ERROR_STATUS_COMPARE_FAILURE = 0x85, ///< Compare Failure (Base 2.4, 4.2.3.3, Figure 107, page 154, PDF page 180)
	XNVME_SPEC_BASE_MEDIA_AND_DATA_INTEGRITY_ERROR_STATUS_ACCESS_DENIED = 0x86, ///< Access Denied (Base 2.4, 4.2.3.3, Figure 107, page 154, PDF page 180)
	XNVME_SPEC_BASE_MEDIA_AND_DATA_INTEGRITY_ERROR_STATUS_DEALLOCATED_OR_UNWRITTEN_LOGICAL_BLOCK = 0x87, ///< Deallocated or Unwritten Logical Block (Base 2.4, 4.2.3.3, Figure 107, page 154, PDF page 180)
	XNVME_SPEC_BASE_MEDIA_AND_DATA_INTEGRITY_ERROR_STATUS_END_TO_END_STORAGE_TAG_CHECK_ERROR = 0x88, ///< End-to-End Storage Tag Check Error (Base 2.4, 4.2.3.3, Figure 107, page 154, PDF page 180)
};

/**
 * The status codes of the type Path Related
 */
enum xnvme_spec_base_path_related_status {
	XNVME_SPEC_BASE_PATH_RELATED_STATUS_INTERNAL_PATH_ERROR = 0x0, ///< Internal Path Error (Base 2.4, 4.2.3.4, Figure 108, page 155, PDF page 181)
	XNVME_SPEC_BASE_PATH_RELATED_STATUS_ASYMMETRIC_ACCESS_PERSISTENT_LOSS = 0x1, ///< Asymmetric Access Persistent Loss (Base 2.4, 4.2.3.4, Figure 108, page 155, PDF page 181)
	XNVME_SPEC_BASE_PATH_RELATED_STATUS_ASYMMETRIC_ACCESS_INACCESSIBLE = 0x2, ///< Asymmetric Access Inaccessible (Base 2.4, 4.2.3.4, Figure 108, page 155, PDF page 181)
	XNVME_SPEC_BASE_PATH_RELATED_STATUS_ASYMMETRIC_ACCESS_TRANSITION = 0x3, ///< Asymmetric Access Transition (Base 2.4, 4.2.3.4, Figure 108, page 155, PDF page 181)
	XNVME_SPEC_BASE_PATH_RELATED_STATUS_CONTROLLER_PATHING_ERROR = 0x60, ///< Controller Pathing Error (Base 2.4, 4.2.3.4, Figure 108, page 155, PDF page 181)
	XNVME_SPEC_BASE_PATH_RELATED_STATUS_HOST_PATHING_ERROR = 0x70, ///< Host Pathing Error (Base 2.4, 4.2.3.4, Figure 108, page 155, PDF page 181)
	XNVME_SPEC_BASE_PATH_RELATED_STATUS_COMMAND_ABORTED_BY_HOST = 0x71, ///< Command Aborted By Host (Base 2.4, 4.2.3.4, Figure 108, page 155, PDF page 181)
};

/**
 * The status codes of the type Pull Model DDC Zoning Operations
 */
enum xnvme_spec_base_pull_model_ddc_zoning_operations_status {
	XNVME_SPEC_BASE_PULL_MODEL_DDC_ZONING_OPERATIONS_STATUS_OPERATION_SUCCESSFUL = 0x0, ///< Operation Successful (Base 2.4, 8.3.2.3.8.3.4, Figure 844, page 784, PDF page 814)
	XNVME_SPEC_BASE_PULL_MODEL_DDC_ZONING_OPERATIONS_STATUS_OPERATION_IN_PROGRESS = 0x1, ///< Operation in Progress (Base 2.4, 8.3.2.3.8.3.4, Figure 844, page 784, PDF page 814)
	XNVME_SPEC_BASE_PULL_MODEL_DDC_ZONING_OPERATIONS_STATUS_ZONING_DATA_STRUCTURE_LOCKED = 0x3, ///< Zoning Data Structure Locked (Base 2.4, 8.3.2.3.8.3.4, Figure 844, page 784, PDF page 814)
	XNVME_SPEC_BASE_PULL_MODEL_DDC_ZONING_OPERATIONS_STATUS_ZONEGROUP_ORIGINATOR_INVALID = 0x4, ///< ZoneGroup Originator Invalid (Base 2.4, 8.3.2.3.8.3.4, Figure 844, page 784, PDF page 814)
	XNVME_SPEC_BASE_PULL_MODEL_DDC_ZONING_OPERATIONS_STATUS_ZONEGROUP_CHANGED = 0x5, ///< ZoneGroup Changed (Base 2.4, 8.3.2.3.8.3.4, Figure 844, page 784, PDF page 814)
};

/**
 * Common Command Format (Base 2.4, 4.1.1, Figure 93, page 140, PDF page 166)
 */
struct __attribute__((packed)) xnvme_spec_base_common_command_format {
	uint32_t cdw0; ///< Command Dword 0, bytes 3:0, see XNVME_SPEC_BASE_COMMON_COMMAND_FORMAT_CDW0_*
	uint32_t nsid; ///< Namespace Identifier, bytes 7:4
	uint32_t cdw2; ///< Command Dword 2, bytes 11:8
	uint32_t cdw3; ///< Command Dword 3, bytes 15:12
	uint64_t mptr; ///< Metadata Pointer, bytes 23:16
	uint8_t dptr[16]; ///< Data Pointer, bytes 39:24
	uint32_t cdw10; ///< Command Dword 10, bytes 43:40
	uint32_t cdw11; ///< Command Dword 11, bytes 47:44
	uint32_t cdw12; ///< Command Dword 12, bytes 51:48
	uint32_t cdw13; ///< Command Dword 13, bytes 55:52
	uint32_t cdw14; ///< Command Dword 14, bytes 59:56
	uint32_t cdw15; ///< Command Dword 15, bytes 63:60
};

/** Command Identifier (Base 2.4, 4.1.1, Figure 92, page 139, PDF page 165); bits 31:16 of 'cdw0' */
#define XNVME_SPEC_BASE_COMMON_COMMAND_FORMAT_CDW0_CID 0xffff0000
/** PRP or SGL for Data Transfer (Base 2.4, 4.1.1, Figure 92, page 139, PDF page 165); bits 15:14 of 'cdw0' */
#define XNVME_SPEC_BASE_COMMON_COMMAND_FORMAT_CDW0_PSDT 0xc000
/** Fused Operation (Base 2.4, 4.1.1, Figure 92, page 139, PDF page 165); bits 9:8 of 'cdw0' */
#define XNVME_SPEC_BASE_COMMON_COMMAND_FORMAT_CDW0_FUSE 0x300
/** Opcode (Base 2.4, 4.1.1, Figure 92, page 139, PDF page 165); bits 7:0 of 'cdw0' */
#define XNVME_SPEC_BASE_COMMON_COMMAND_FORMAT_CDW0_OPC 0xff

/**
 * The values of PRP or SGL for Data Transfer (Base 2.4, 4.1.1, Figure 92, page 139, PDF page 165)
 */
enum xnvme_spec_base_common_command_format_cdw0_psdt {
	XNVME_SPEC_BASE_COMMON_COMMAND_FORMAT_CDW0_PSDT_PRPS_USED = 0x0, ///< PRPS Used (Base 2.4, 4.1.1, Figure 92, page 139, PDF page 165)
	XNVME_SPEC_BASE_COMMON_COMMAND_FORMAT_CDW0_PSDT_SGLS_USED_MPTR_ADDRESS = 0x1, ///< SGLs Used MPTR Address (Base 2.4, 4.1.1, Figure 92, page 139, PDF page 165)
	XNVME_SPEC_BASE_COMMON_COMMAND_FORMAT_CDW0_PSDT_SGLS_USED_MPTR_SGL_SEGMENT = 0x2, ///< SGLs Used MPTR SGL Segment (Base 2.4, 4.1.1, Figure 92, page 139, PDF page 165)
};

/**
 * Common Command Format - Vendor Specific Commands (Optional) (Base 2.4, 4.1.1, Figure 94, page 143, PDF page 169)
 */
struct __attribute__((packed)) xnvme_spec_base_vendor_specific_commands {
	uint32_t cdw0; ///< Command Dword 0, bytes 3:0, see XNVME_SPEC_BASE_VENDOR_SPECIFIC_COMMANDS_CDW0_*
	uint32_t nsid; ///< Namespace Identifier, bytes 7:4
	uint64_t rsvd8; ///< Reserved, bytes 15:8
	uint8_t mdptr[24]; ///< Metadata and Data Pointers, bytes 39:16
	uint32_t ndt; ///< Number of Dwords in Data Transfer, bytes 43:40
	uint32_t ndm; ///< Number of Dwords in Metadata Transfer, bytes 47:44
	uint32_t cdw12; ///< Command Dword 12, bytes 51:48
	uint32_t cdw13; ///< Command Dword 13, bytes 55:52
	uint32_t cdw14; ///< Command Dword 14, bytes 59:56
	uint32_t cdw15; ///< Command Dword 15, bytes 63:60
};

/** Command Identifier (Base 2.4, 4.1.1, Figure 92, page 139, PDF page 165); bits 31:16 of 'cdw0' */
#define XNVME_SPEC_BASE_VENDOR_SPECIFIC_COMMANDS_CDW0_CID 0xffff0000
/** PRP or SGL for Data Transfer (Base 2.4, 4.1.1, Figure 92, page 139, PDF page 165); bits 15:14 of 'cdw0' */
#define XNVME_SPEC_BASE_VENDOR_SPECIFIC_COMMANDS_CDW0_PSDT 0xc000
/** Fused Operation (Base 2.4, 4.1.1, Figure 92, page 139, PDF page 165); bits 9:8 of 'cdw0' */
#define XNVME_SPEC_BASE_VENDOR_SPECIFIC_COMMANDS_CDW0_FUSE 0x300
/** Opcode (Base 2.4, 4.1.1, Figure 92, page 139, PDF page 165); bits 7:0 of 'cdw0' */
#define XNVME_SPEC_BASE_VENDOR_SPECIFIC_COMMANDS_CDW0_OPC 0xff

/**
 * The values of PRP or SGL for Data Transfer (Base 2.4, 4.1.1, Figure 92, page 139, PDF page 165)
 */
enum xnvme_spec_base_vendor_specific_commands_cdw0_psdt {
	XNVME_SPEC_BASE_VENDOR_SPECIFIC_COMMANDS_CDW0_PSDT_PRPS_USED = 0x0, ///< PRPS Used (Base 2.4, 4.1.1, Figure 92, page 139, PDF page 165)
	XNVME_SPEC_BASE_VENDOR_SPECIFIC_COMMANDS_CDW0_PSDT_SGLS_USED_MPTR_ADDRESS = 0x1, ///< SGLs Used MPTR Address (Base 2.4, 4.1.1, Figure 92, page 139, PDF page 165)
	XNVME_SPEC_BASE_VENDOR_SPECIFIC_COMMANDS_CDW0_PSDT_SGLS_USED_MPTR_SGL_SEGMENT = 0x2, ///< SGLs Used MPTR SGL Segment (Base 2.4, 4.1.1, Figure 92, page 139, PDF page 165)
};

/**
 * Fabrics Command - Submission Queue Entry Format (Base 2.4, 4.1.2, Figure 95, page 143, PDF page 169)
 */
struct __attribute__((packed)) xnvme_spec_base_submission_queue_entry_format {
	uint32_t cdw0; ///< Command Dword 0, bytes 3:0
	uint8_t fctype; ///< Fabrics Command Type, byte 4, see XNVME_SPEC_BASE_SUBMISSION_QUEUE_ENTRY_FORMAT_FCTYPE_*
	uint8_t rsvd5[19]; ///< Reserved, bytes 23:5
	uint8_t sgl1[16]; ///< SGL Descriptor 1, bytes 39:24
	uint8_t fcts[24]; ///< Fabrics Command Type Specific, bytes 63:40
};

/** Function (Base 2.4, 4.1.2, Figure 95, page 143, PDF page 169); bits 7:2 of 'fctype' */
#define XNVME_SPEC_BASE_SUBMISSION_QUEUE_ENTRY_FORMAT_FCTYPE_FN 0xfc
/** Data Transfer Direction (Base 2.4, 4.1.2, Figure 95, page 143, PDF page 169); bits 1:0 of 'fctype' */
#define XNVME_SPEC_BASE_SUBMISSION_QUEUE_ENTRY_FORMAT_FCTYPE_DTD 0x3

/**
 * The values of Data Transfer Direction (Base 2.4, 4.1.2, Figure 95, page 143, PDF page 169)
 */
enum xnvme_spec_base_submission_queue_entry_format_fctype_dtd {
	XNVME_SPEC_BASE_SUBMISSION_QUEUE_ENTRY_FORMAT_FCTYPE_DTD_NO_DATA_TRANSFER = 0x0, ///< No Data Transfer (Base 2.4, 4.1.2, Figure 95, page 143, PDF page 169)
	XNVME_SPEC_BASE_SUBMISSION_QUEUE_ENTRY_FORMAT_FCTYPE_DTD_HOST_TO_CONTROLLER_TRANSFER = 0x1, ///< Host to Controller Transfer (Base 2.4, 4.1.2, Figure 95, page 143, PDF page 169)
	XNVME_SPEC_BASE_SUBMISSION_QUEUE_ENTRY_FORMAT_FCTYPE_DTD_CONTROLLER_TO_HOST_TRANSFER = 0x2, ///< Controller to Host Transfer (Base 2.4, 4.1.2, Figure 95, page 143, PDF page 169)
};

/**
 * Fabrics Response - Completion Queue Entry Format (Base 2.4, 4.2.2, Figure 100, page 145, PDF page 171)
 */
struct __attribute__((packed)) xnvme_spec_base_completion_queue_entry_format {
	uint64_t frts; ///< Fabrics Response Type Specific, bytes 7:0
	uint16_t sqhd; ///< SQ Head Pointer, bytes 9:8
	uint16_t rsvd10; ///< Reserved, bytes 11:10
	uint16_t cid; ///< Command Identifier, bytes 13:12
	uint16_t sts; ///< Status Info, bytes 15:14, see XNVME_SPEC_BASE_COMPLETION_QUEUE_ENTRY_FORMAT_STS_*
};

/** Status (Base 2.4, 4.2.2, Figure 100, page 145, PDF page 171); bits 15:1 of 'sts' */
#define XNVME_SPEC_BASE_COMPLETION_QUEUE_ENTRY_FORMAT_STS_STATUS 0xfffe

/**
 * Generic SGL Descriptor Format (Base 2.4, 4.3.2, Figure 116, page 161, PDF page 187)
 */
struct __attribute__((packed)) xnvme_spec_base_generic_sgl_descriptor_format {
	uint8_t dts[15]; ///< Descriptor Type Specific, bytes 14:0
	uint8_t sglid; ///< SGL Identifier, byte 15, see XNVME_SPEC_BASE_GENERIC_SGL_DESCRIPTOR_FORMAT_SGLID_*
};

/** SGL Descriptor Type (Base 2.4, 4.3.2, Figure 116, page 161, PDF page 187); bits 7:4 of 'sglid' */
#define XNVME_SPEC_BASE_GENERIC_SGL_DESCRIPTOR_FORMAT_SGLID_SGLDT 0xf0
/** SGL Descriptor Sub Type (Base 2.4, 4.3.2, Figure 116, page 161, PDF page 187); bits 3:0 of 'sglid' */
#define XNVME_SPEC_BASE_GENERIC_SGL_DESCRIPTOR_FORMAT_SGLID_SGLDST 0xf

/**
 * The values of SGL Descriptor Type (Base 2.4, 4.3.2, Figure 117, page 161, PDF page 187)
 */
enum xnvme_spec_base_generic_sgl_descriptor_format_sglid_sgldt {
	XNVME_SPEC_BASE_GENERIC_SGL_DESCRIPTOR_FORMAT_SGLID_SGLDT_SGL_DATA_BLOCK_DESCRIPTOR = 0x0, ///< SGL Data Block descriptor (Base 2.4, 4.3.2, Figure 117, page 161, PDF page 187)
	XNVME_SPEC_BASE_GENERIC_SGL_DESCRIPTOR_FORMAT_SGLID_SGLDT_SGL_BIT_BUCKET_DESCRIPTOR = 0x1, ///< SGL Bit Bucket descriptor (Base 2.4, 4.3.2, Figure 117, page 161, PDF page 187)
	XNVME_SPEC_BASE_GENERIC_SGL_DESCRIPTOR_FORMAT_SGLID_SGLDT_SGL_SEGMENT_DESCRIPTOR = 0x2, ///< SGL Segment descriptor (Base 2.4, 4.3.2, Figure 117, page 161, PDF page 187)
	XNVME_SPEC_BASE_GENERIC_SGL_DESCRIPTOR_FORMAT_SGLID_SGLDT_SGL_LAST_SEGMENT_DESCRIPTOR = 0x3, ///< SGL Last Segment descriptor (Base 2.4, 4.3.2, Figure 117, page 161, PDF page 187)
	XNVME_SPEC_BASE_GENERIC_SGL_DESCRIPTOR_FORMAT_SGLID_SGLDT_KEYED_SGL_DATA_BLOCK_DESCRIPTOR = 0x4, ///< Keyed SGL Data Block descriptor (Base 2.4, 4.3.2, Figure 117, page 161, PDF page 187)
	XNVME_SPEC_BASE_GENERIC_SGL_DESCRIPTOR_FORMAT_SGLID_SGLDT_TRANSPORT_SGL_DATA_BLOCK_DESCRIPTOR = 0x5, ///< Transport SGL Data Block descriptor (Base 2.4, 4.3.2, Figure 117, page 161, PDF page 187)
};

/**
 * The values of SGL Descriptor Sub Type (Base 2.4, 4.3.2, Figure 118, page 162, PDF page 188)
 */
enum xnvme_spec_base_generic_sgl_descriptor_format_sglid_sgldst {
	XNVME_SPEC_BASE_GENERIC_SGL_DESCRIPTOR_FORMAT_SGLID_SGLDST_0H_2H_3H_4H = 0x0, ///< 0h, 2h, 3h, 4h (Base 2.4, 4.3.2, Figure 118, page 162, PDF page 188)
	XNVME_SPEC_BASE_GENERIC_SGL_DESCRIPTOR_FORMAT_SGLID_SGLDST_ALL_OTHER_VALUES = 0x0, ///< All other values (Base 2.4, 4.3.2, Figure 118, page 162, PDF page 188)
	XNVME_SPEC_BASE_GENERIC_SGL_DESCRIPTOR_FORMAT_SGLID_SGLDST_0H_2H_3H = 0x1, ///< 0h, 2h, 3h (Base 2.4, 4.3.2, Figure 118, page 162, PDF page 188)
	XNVME_SPEC_BASE_GENERIC_SGL_DESCRIPTOR_FORMAT_SGLID_SGLDST_ALL_OTHER_VALUES_1H = 0x1, ///< All other values (Base 2.4, 4.3.2, Figure 118, page 162, PDF page 188)
};

/**
 * SGL Data Block descriptor (Base 2.4, 4.3.2, Figure 119, page 162, PDF page 188)
 */
struct __attribute__((packed)) xnvme_spec_base_sgl_data_block_descriptor {
	uint64_t addr; ///< Address, bytes 7:0
	uint32_t len; ///< Length, bytes 11:8
	uint8_t rsvd12[3]; ///< Reserved, bytes 14:12
	uint8_t sglid; ///< SGL Identifier, byte 15, see XNVME_SPEC_BASE_SGL_DATA_BLOCK_DESCRIPTOR_SGLID_*
};

/** SGL Descriptor Type (Base 2.4, 4.3.2, Figure 119, page 162, PDF page 188); bits 7:4 of 'sglid' */
#define XNVME_SPEC_BASE_SGL_DATA_BLOCK_DESCRIPTOR_SGLID_SGLDT 0xf0
/** SGL Descriptor Sub Type (Base 2.4, 4.3.2, Figure 119, page 162, PDF page 188); bits 3:0 of 'sglid' */
#define XNVME_SPEC_BASE_SGL_DATA_BLOCK_DESCRIPTOR_SGLID_SGLDST 0xf

/**
 * The values of SGL Descriptor Type (Base 2.4, 4.3.2, Figure 117, page 161, PDF page 187)
 */
enum xnvme_spec_base_sgl_data_block_descriptor_sglid_sgldt {
	XNVME_SPEC_BASE_SGL_DATA_BLOCK_DESCRIPTOR_SGLID_SGLDT_SGL_DATA_BLOCK_DESCRIPTOR = 0x0, ///< SGL Data Block descriptor (Base 2.4, 4.3.2, Figure 117, page 161, PDF page 187)
	XNVME_SPEC_BASE_SGL_DATA_BLOCK_DESCRIPTOR_SGLID_SGLDT_SGL_BIT_BUCKET_DESCRIPTOR = 0x1, ///< SGL Bit Bucket descriptor (Base 2.4, 4.3.2, Figure 117, page 161, PDF page 187)
	XNVME_SPEC_BASE_SGL_DATA_BLOCK_DESCRIPTOR_SGLID_SGLDT_SGL_SEGMENT_DESCRIPTOR = 0x2, ///< SGL Segment descriptor (Base 2.4, 4.3.2, Figure 117, page 161, PDF page 187)
	XNVME_SPEC_BASE_SGL_DATA_BLOCK_DESCRIPTOR_SGLID_SGLDT_SGL_LAST_SEGMENT_DESCRIPTOR = 0x3, ///< SGL Last Segment descriptor (Base 2.4, 4.3.2, Figure 117, page 161, PDF page 187)
	XNVME_SPEC_BASE_SGL_DATA_BLOCK_DESCRIPTOR_SGLID_SGLDT_KEYED_SGL_DATA_BLOCK_DESCRIPTOR = 0x4, ///< Keyed SGL Data Block descriptor (Base 2.4, 4.3.2, Figure 117, page 161, PDF page 187)
	XNVME_SPEC_BASE_SGL_DATA_BLOCK_DESCRIPTOR_SGLID_SGLDT_TRANSPORT_SGL_DATA_BLOCK_DESCRIPTOR = 0x5, ///< Transport SGL Data Block descriptor (Base 2.4, 4.3.2, Figure 117, page 161, PDF page 187)
};

/**
 * The values of SGL Descriptor Sub Type (Base 2.4, 4.3.2, Figure 118, page 162, PDF page 188)
 */
enum xnvme_spec_base_sgl_data_block_descriptor_sglid_sgldst {
	XNVME_SPEC_BASE_SGL_DATA_BLOCK_DESCRIPTOR_SGLID_SGLDST_0H_2H_3H_4H = 0x0, ///< 0h, 2h, 3h, 4h (Base 2.4, 4.3.2, Figure 118, page 162, PDF page 188)
	XNVME_SPEC_BASE_SGL_DATA_BLOCK_DESCRIPTOR_SGLID_SGLDST_ALL_OTHER_VALUES = 0x0, ///< All other values (Base 2.4, 4.3.2, Figure 118, page 162, PDF page 188)
	XNVME_SPEC_BASE_SGL_DATA_BLOCK_DESCRIPTOR_SGLID_SGLDST_0H_2H_3H = 0x1, ///< 0h, 2h, 3h (Base 2.4, 4.3.2, Figure 118, page 162, PDF page 188)
	XNVME_SPEC_BASE_SGL_DATA_BLOCK_DESCRIPTOR_SGLID_SGLDST_ALL_OTHER_VALUES_1H = 0x1, ///< All other values (Base 2.4, 4.3.2, Figure 118, page 162, PDF page 188)
};

/**
 * SGL Bit Bucket descriptor (Base 2.4, 4.3.2, Figure 120, page 163, PDF page 189)
 */
struct __attribute__((packed)) xnvme_spec_base_sgl_bit_bucket_descriptor {
	uint64_t rsvd0; ///< Reserved, bytes 7:0
	uint32_t len; ///< Length, bytes 11:8
	uint8_t rsvd12[3]; ///< Reserved, bytes 14:12
	uint8_t sglid; ///< SGL Identifier, byte 15, see XNVME_SPEC_BASE_SGL_BIT_BUCKET_DESCRIPTOR_SGLID_*
};

/** SGL Descriptor Type (Base 2.4, 4.3.2, Figure 120, page 163, PDF page 189); bits 7:4 of 'sglid' */
#define XNVME_SPEC_BASE_SGL_BIT_BUCKET_DESCRIPTOR_SGLID_SGLDT 0xf0
/** SGL Descriptor Sub Type (Base 2.4, 4.3.2, Figure 120, page 163, PDF page 189); bits 3:0 of 'sglid' */
#define XNVME_SPEC_BASE_SGL_BIT_BUCKET_DESCRIPTOR_SGLID_SGLDST 0xf

/**
 * The values of SGL Descriptor Type (Base 2.4, 4.3.2, Figure 117, page 161, PDF page 187)
 */
enum xnvme_spec_base_sgl_bit_bucket_descriptor_sglid_sgldt {
	XNVME_SPEC_BASE_SGL_BIT_BUCKET_DESCRIPTOR_SGLID_SGLDT_SGL_DATA_BLOCK_DESCRIPTOR = 0x0, ///< SGL Data Block descriptor (Base 2.4, 4.3.2, Figure 117, page 161, PDF page 187)
	XNVME_SPEC_BASE_SGL_BIT_BUCKET_DESCRIPTOR_SGLID_SGLDT_SGL_BIT_BUCKET_DESCRIPTOR = 0x1, ///< SGL Bit Bucket descriptor (Base 2.4, 4.3.2, Figure 117, page 161, PDF page 187)
	XNVME_SPEC_BASE_SGL_BIT_BUCKET_DESCRIPTOR_SGLID_SGLDT_SGL_SEGMENT_DESCRIPTOR = 0x2, ///< SGL Segment descriptor (Base 2.4, 4.3.2, Figure 117, page 161, PDF page 187)
	XNVME_SPEC_BASE_SGL_BIT_BUCKET_DESCRIPTOR_SGLID_SGLDT_SGL_LAST_SEGMENT_DESCRIPTOR = 0x3, ///< SGL Last Segment descriptor (Base 2.4, 4.3.2, Figure 117, page 161, PDF page 187)
	XNVME_SPEC_BASE_SGL_BIT_BUCKET_DESCRIPTOR_SGLID_SGLDT_KEYED_SGL_DATA_BLOCK_DESCRIPTOR = 0x4, ///< Keyed SGL Data Block descriptor (Base 2.4, 4.3.2, Figure 117, page 161, PDF page 187)
	XNVME_SPEC_BASE_SGL_BIT_BUCKET_DESCRIPTOR_SGLID_SGLDT_TRANSPORT_SGL_DATA_BLOCK_DESCRIPTOR = 0x5, ///< Transport SGL Data Block descriptor (Base 2.4, 4.3.2, Figure 117, page 161, PDF page 187)
};

/**
 * The values of SGL Descriptor Sub Type (Base 2.4, 4.3.2, Figure 118, page 162, PDF page 188)
 */
enum xnvme_spec_base_sgl_bit_bucket_descriptor_sglid_sgldst {
	XNVME_SPEC_BASE_SGL_BIT_BUCKET_DESCRIPTOR_SGLID_SGLDST_0H_2H_3H_4H = 0x0, ///< 0h, 2h, 3h, 4h (Base 2.4, 4.3.2, Figure 118, page 162, PDF page 188)
	XNVME_SPEC_BASE_SGL_BIT_BUCKET_DESCRIPTOR_SGLID_SGLDST_ALL_OTHER_VALUES = 0x0, ///< All other values (Base 2.4, 4.3.2, Figure 118, page 162, PDF page 188)
	XNVME_SPEC_BASE_SGL_BIT_BUCKET_DESCRIPTOR_SGLID_SGLDST_0H_2H_3H = 0x1, ///< 0h, 2h, 3h (Base 2.4, 4.3.2, Figure 118, page 162, PDF page 188)
	XNVME_SPEC_BASE_SGL_BIT_BUCKET_DESCRIPTOR_SGLID_SGLDST_ALL_OTHER_VALUES_1H = 0x1, ///< All other values (Base 2.4, 4.3.2, Figure 118, page 162, PDF page 188)
};

/**
 * SGL Segment descriptor (Base 2.4, 4.3.2, Figure 121, page 163, PDF page 189)
 */
struct __attribute__((packed)) xnvme_spec_base_sgl_segment_descriptor {
	uint64_t addr; ///< Address, bytes 7:0
	uint32_t len; ///< Length, bytes 11:8
	uint8_t rsvd12[3]; ///< Reserved, bytes 14:12
	uint8_t sglid; ///< SGL Identifier, byte 15, see XNVME_SPEC_BASE_SGL_SEGMENT_DESCRIPTOR_SGLID_*
};

/** SGL Descriptor Type (Base 2.4, 4.3.2, Figure 121, page 163, PDF page 189); bits 7:4 of 'sglid' */
#define XNVME_SPEC_BASE_SGL_SEGMENT_DESCRIPTOR_SGLID_SGLDT 0xf0
/** SGL Descriptor Sub Type (Base 2.4, 4.3.2, Figure 121, page 163, PDF page 189); bits 3:0 of 'sglid' */
#define XNVME_SPEC_BASE_SGL_SEGMENT_DESCRIPTOR_SGLID_SGDST 0xf

/**
 * The values of SGL Descriptor Type (Base 2.4, 4.3.2, Figure 117, page 161, PDF page 187)
 */
enum xnvme_spec_base_sgl_segment_descriptor_sglid_sgldt {
	XNVME_SPEC_BASE_SGL_SEGMENT_DESCRIPTOR_SGLID_SGLDT_SGL_DATA_BLOCK_DESCRIPTOR = 0x0, ///< SGL Data Block descriptor (Base 2.4, 4.3.2, Figure 117, page 161, PDF page 187)
	XNVME_SPEC_BASE_SGL_SEGMENT_DESCRIPTOR_SGLID_SGLDT_SGL_BIT_BUCKET_DESCRIPTOR = 0x1, ///< SGL Bit Bucket descriptor (Base 2.4, 4.3.2, Figure 117, page 161, PDF page 187)
	XNVME_SPEC_BASE_SGL_SEGMENT_DESCRIPTOR_SGLID_SGLDT_SGL_SEGMENT_DESCRIPTOR = 0x2, ///< SGL Segment descriptor (Base 2.4, 4.3.2, Figure 117, page 161, PDF page 187)
	XNVME_SPEC_BASE_SGL_SEGMENT_DESCRIPTOR_SGLID_SGLDT_SGL_LAST_SEGMENT_DESCRIPTOR = 0x3, ///< SGL Last Segment descriptor (Base 2.4, 4.3.2, Figure 117, page 161, PDF page 187)
	XNVME_SPEC_BASE_SGL_SEGMENT_DESCRIPTOR_SGLID_SGLDT_KEYED_SGL_DATA_BLOCK_DESCRIPTOR = 0x4, ///< Keyed SGL Data Block descriptor (Base 2.4, 4.3.2, Figure 117, page 161, PDF page 187)
	XNVME_SPEC_BASE_SGL_SEGMENT_DESCRIPTOR_SGLID_SGLDT_TRANSPORT_SGL_DATA_BLOCK_DESCRIPTOR = 0x5, ///< Transport SGL Data Block descriptor (Base 2.4, 4.3.2, Figure 117, page 161, PDF page 187)
};

/**
 * The values of SGL Descriptor Sub Type (Base 2.4, 4.3.2, Figure 118, page 162, PDF page 188)
 */
enum xnvme_spec_base_sgl_segment_descriptor_sglid_sgdst {
	XNVME_SPEC_BASE_SGL_SEGMENT_DESCRIPTOR_SGLID_SGDST_0H_2H_3H_4H = 0x0, ///< 0h, 2h, 3h, 4h (Base 2.4, 4.3.2, Figure 118, page 162, PDF page 188)
	XNVME_SPEC_BASE_SGL_SEGMENT_DESCRIPTOR_SGLID_SGDST_ALL_OTHER_VALUES = 0x0, ///< All other values (Base 2.4, 4.3.2, Figure 118, page 162, PDF page 188)
	XNVME_SPEC_BASE_SGL_SEGMENT_DESCRIPTOR_SGLID_SGDST_0H_2H_3H = 0x1, ///< 0h, 2h, 3h (Base 2.4, 4.3.2, Figure 118, page 162, PDF page 188)
	XNVME_SPEC_BASE_SGL_SEGMENT_DESCRIPTOR_SGLID_SGDST_ALL_OTHER_VALUES_1H = 0x1, ///< All other values (Base 2.4, 4.3.2, Figure 118, page 162, PDF page 188)
};

/**
 * SGL Last Segment descriptor (Base 2.4, 4.3.2, Figure 122, page 164, PDF page 190)
 */
struct __attribute__((packed)) xnvme_spec_base_sgl_last_segment_descriptor {
	uint64_t addr; ///< Address, bytes 7:0
	uint32_t len; ///< Length, bytes 11:8
	uint8_t rsvd12[3]; ///< Reserved, bytes 14:12
	uint8_t sglid; ///< SGL Identifier, byte 15, see XNVME_SPEC_BASE_SGL_LAST_SEGMENT_DESCRIPTOR_SGLID_*
};

/** SGL Descriptor Type (Base 2.4, 4.3.2, Figure 122, page 164, PDF page 190); bits 7:4 of 'sglid' */
#define XNVME_SPEC_BASE_SGL_LAST_SEGMENT_DESCRIPTOR_SGLID_SGLDT 0xf0
/** SGL Descriptor Sub Type (Base 2.4, 4.3.2, Figure 122, page 164, PDF page 190); bits 3:0 of 'sglid' */
#define XNVME_SPEC_BASE_SGL_LAST_SEGMENT_DESCRIPTOR_SGLID_SGLDST 0xf

/**
 * The values of SGL Descriptor Type (Base 2.4, 4.3.2, Figure 117, page 161, PDF page 187)
 */
enum xnvme_spec_base_sgl_last_segment_descriptor_sglid_sgldt {
	XNVME_SPEC_BASE_SGL_LAST_SEGMENT_DESCRIPTOR_SGLID_SGLDT_SGL_DATA_BLOCK_DESCRIPTOR = 0x0, ///< SGL Data Block descriptor (Base 2.4, 4.3.2, Figure 117, page 161, PDF page 187)
	XNVME_SPEC_BASE_SGL_LAST_SEGMENT_DESCRIPTOR_SGLID_SGLDT_SGL_BIT_BUCKET_DESCRIPTOR = 0x1, ///< SGL Bit Bucket descriptor (Base 2.4, 4.3.2, Figure 117, page 161, PDF page 187)
	XNVME_SPEC_BASE_SGL_LAST_SEGMENT_DESCRIPTOR_SGLID_SGLDT_SGL_SEGMENT_DESCRIPTOR = 0x2, ///< SGL Segment descriptor (Base 2.4, 4.3.2, Figure 117, page 161, PDF page 187)
	XNVME_SPEC_BASE_SGL_LAST_SEGMENT_DESCRIPTOR_SGLID_SGLDT_SGL_LAST_SEGMENT_DESCRIPTOR = 0x3, ///< SGL Last Segment descriptor (Base 2.4, 4.3.2, Figure 117, page 161, PDF page 187)
	XNVME_SPEC_BASE_SGL_LAST_SEGMENT_DESCRIPTOR_SGLID_SGLDT_KEYED_SGL_DATA_BLOCK_DESCRIPTOR = 0x4, ///< Keyed SGL Data Block descriptor (Base 2.4, 4.3.2, Figure 117, page 161, PDF page 187)
	XNVME_SPEC_BASE_SGL_LAST_SEGMENT_DESCRIPTOR_SGLID_SGLDT_TRANSPORT_SGL_DATA_BLOCK_DESCRIPTOR = 0x5, ///< Transport SGL Data Block descriptor (Base 2.4, 4.3.2, Figure 117, page 161, PDF page 187)
};

/**
 * The values of SGL Descriptor Sub Type (Base 2.4, 4.3.2, Figure 118, page 162, PDF page 188)
 */
enum xnvme_spec_base_sgl_last_segment_descriptor_sglid_sgldst {
	XNVME_SPEC_BASE_SGL_LAST_SEGMENT_DESCRIPTOR_SGLID_SGLDST_0H_2H_3H_4H = 0x0, ///< 0h, 2h, 3h, 4h (Base 2.4, 4.3.2, Figure 118, page 162, PDF page 188)
	XNVME_SPEC_BASE_SGL_LAST_SEGMENT_DESCRIPTOR_SGLID_SGLDST_ALL_OTHER_VALUES = 0x0, ///< All other values (Base 2.4, 4.3.2, Figure 118, page 162, PDF page 188)
	XNVME_SPEC_BASE_SGL_LAST_SEGMENT_DESCRIPTOR_SGLID_SGLDST_0H_2H_3H = 0x1, ///< 0h, 2h, 3h (Base 2.4, 4.3.2, Figure 118, page 162, PDF page 188)
	XNVME_SPEC_BASE_SGL_LAST_SEGMENT_DESCRIPTOR_SGLID_SGLDST_ALL_OTHER_VALUES_1H = 0x1, ///< All other values (Base 2.4, 4.3.2, Figure 118, page 162, PDF page 188)
};

/**
 * Keyed SGL Data Block descriptor (Base 2.4, 4.3.2, Figure 123, page 164, PDF page 190)
 */
struct __attribute__((packed)) xnvme_spec_base_keyed_sgl_data_block_descriptor {
	uint64_t addr; ///< Address, bytes 7:0
	uint8_t len[3]; ///< Length, bytes 10:8
	uint32_t key; ///< Key, bytes 14:11
	uint8_t sglid; ///< SGL Identifier, byte 15, see XNVME_SPEC_BASE_KEYED_SGL_DATA_BLOCK_DESCRIPTOR_SGLID_*
};

/** SGL Descriptor Type (Base 2.4, 4.3.2, Figure 123, page 164, PDF page 190); bits 7:4 of 'sglid' */
#define XNVME_SPEC_BASE_KEYED_SGL_DATA_BLOCK_DESCRIPTOR_SGLID_SGLDT 0xf0
/** SGL Descriptor Sub Type (Base 2.4, 4.3.2, Figure 123, page 164, PDF page 190); bits 3:0 of 'sglid' */
#define XNVME_SPEC_BASE_KEYED_SGL_DATA_BLOCK_DESCRIPTOR_SGLID_SGLDST 0xf

/**
 * The values of SGL Descriptor Type (Base 2.4, 4.3.2, Figure 117, page 161, PDF page 187)
 */
enum xnvme_spec_base_keyed_sgl_data_block_descriptor_sglid_sgldt {
	XNVME_SPEC_BASE_KEYED_SGL_DATA_BLOCK_DESCRIPTOR_SGLID_SGLDT_SGL_DATA_BLOCK_DESCRIPTOR = 0x0, ///< SGL Data Block descriptor (Base 2.4, 4.3.2, Figure 117, page 161, PDF page 187)
	XNVME_SPEC_BASE_KEYED_SGL_DATA_BLOCK_DESCRIPTOR_SGLID_SGLDT_SGL_BIT_BUCKET_DESCRIPTOR = 0x1, ///< SGL Bit Bucket descriptor (Base 2.4, 4.3.2, Figure 117, page 161, PDF page 187)
	XNVME_SPEC_BASE_KEYED_SGL_DATA_BLOCK_DESCRIPTOR_SGLID_SGLDT_SGL_SEGMENT_DESCRIPTOR = 0x2, ///< SGL Segment descriptor (Base 2.4, 4.3.2, Figure 117, page 161, PDF page 187)
	XNVME_SPEC_BASE_KEYED_SGL_DATA_BLOCK_DESCRIPTOR_SGLID_SGLDT_SGL_LAST_SEGMENT_DESCRIPTOR = 0x3, ///< SGL Last Segment descriptor (Base 2.4, 4.3.2, Figure 117, page 161, PDF page 187)
	XNVME_SPEC_BASE_KEYED_SGL_DATA_BLOCK_DESCRIPTOR_SGLID_SGLDT_KEYED_SGL_DATA_BLOCK_DESCRIPTOR = 0x4, ///< Keyed SGL Data Block descriptor (Base 2.4, 4.3.2, Figure 117, page 161, PDF page 187)
	XNVME_SPEC_BASE_KEYED_SGL_DATA_BLOCK_DESCRIPTOR_SGLID_SGLDT_TRANSPORT_SGL_DATA_BLOCK_DESCRIPTOR = 0x5, ///< Transport SGL Data Block descriptor (Base 2.4, 4.3.2, Figure 117, page 161, PDF page 187)
};

/**
 * The values of SGL Descriptor Sub Type (Base 2.4, 4.3.2, Figure 118, page 162, PDF page 188)
 */
enum xnvme_spec_base_keyed_sgl_data_block_descriptor_sglid_sgldst {
	XNVME_SPEC_BASE_KEYED_SGL_DATA_BLOCK_DESCRIPTOR_SGLID_SGLDST_0H_2H_3H_4H = 0x0, ///< 0h, 2h, 3h, 4h (Base 2.4, 4.3.2, Figure 118, page 162, PDF page 188)
	XNVME_SPEC_BASE_KEYED_SGL_DATA_BLOCK_DESCRIPTOR_SGLID_SGLDST_ALL_OTHER_VALUES = 0x0, ///< All other values (Base 2.4, 4.3.2, Figure 118, page 162, PDF page 188)
	XNVME_SPEC_BASE_KEYED_SGL_DATA_BLOCK_DESCRIPTOR_SGLID_SGLDST_0H_2H_3H = 0x1, ///< 0h, 2h, 3h (Base 2.4, 4.3.2, Figure 118, page 162, PDF page 188)
	XNVME_SPEC_BASE_KEYED_SGL_DATA_BLOCK_DESCRIPTOR_SGLID_SGLDST_ALL_OTHER_VALUES_1H = 0x1, ///< All other values (Base 2.4, 4.3.2, Figure 118, page 162, PDF page 188)
};

/**
 * Transport SGL Data Block descriptor (Base 2.4, 4.3.2, Figure 124, page 164, PDF page 190)
 */
struct __attribute__((packed)) xnvme_spec_base_transport_sgl_data_block_descriptor {
	uint64_t rsvd0; ///< Reserved, bytes 7:0
	uint32_t len; ///< Length, bytes 11:8
	uint8_t rsvd12[3]; ///< Reserved, bytes 14:12
	uint8_t sglid; ///< SGL Identifier, byte 15, see XNVME_SPEC_BASE_TRANSPORT_SGL_DATA_BLOCK_DESCRIPTOR_SGLID_*
};

/** SGL Descriptor Type (Base 2.4, 4.3.2, Figure 124, page 164, PDF page 190); bits 7:4 of 'sglid' */
#define XNVME_SPEC_BASE_TRANSPORT_SGL_DATA_BLOCK_DESCRIPTOR_SGLID_SGLDT 0xf0
/** SGL Descriptor Sub Type (Base 2.4, 4.3.2, Figure 124, page 164, PDF page 190); bits 3:0 of 'sglid' */
#define XNVME_SPEC_BASE_TRANSPORT_SGL_DATA_BLOCK_DESCRIPTOR_SGLID_SGLDST 0xf

/**
 * The values of SGL Descriptor Type (Base 2.4, 4.3.2, Figure 117, page 161, PDF page 187)
 */
enum xnvme_spec_base_transport_sgl_data_block_descriptor_sglid_sgldt {
	XNVME_SPEC_BASE_TRANSPORT_SGL_DATA_BLOCK_DESCRIPTOR_SGLID_SGLDT_SGL_DATA_BLOCK_DESCRIPTOR = 0x0, ///< SGL Data Block descriptor (Base 2.4, 4.3.2, Figure 117, page 161, PDF page 187)
	XNVME_SPEC_BASE_TRANSPORT_SGL_DATA_BLOCK_DESCRIPTOR_SGLID_SGLDT_SGL_BIT_BUCKET_DESCRIPTOR = 0x1, ///< SGL Bit Bucket descriptor (Base 2.4, 4.3.2, Figure 117, page 161, PDF page 187)
	XNVME_SPEC_BASE_TRANSPORT_SGL_DATA_BLOCK_DESCRIPTOR_SGLID_SGLDT_SGL_SEGMENT_DESCRIPTOR = 0x2, ///< SGL Segment descriptor (Base 2.4, 4.3.2, Figure 117, page 161, PDF page 187)
	XNVME_SPEC_BASE_TRANSPORT_SGL_DATA_BLOCK_DESCRIPTOR_SGLID_SGLDT_SGL_LAST_SEGMENT_DESCRIPTOR = 0x3, ///< SGL Last Segment descriptor (Base 2.4, 4.3.2, Figure 117, page 161, PDF page 187)
	XNVME_SPEC_BASE_TRANSPORT_SGL_DATA_BLOCK_DESCRIPTOR_SGLID_SGLDT_KEYED_SGL_DATA_BLOCK_DESCRIPTOR = 0x4, ///< Keyed SGL Data Block descriptor (Base 2.4, 4.3.2, Figure 117, page 161, PDF page 187)
	XNVME_SPEC_BASE_TRANSPORT_SGL_DATA_BLOCK_DESCRIPTOR_SGLID_SGLDT_TRANSPORT_SGL_DATA_BLOCK_DESCRIPTOR = 0x5, ///< Transport SGL Data Block descriptor (Base 2.4, 4.3.2, Figure 117, page 161, PDF page 187)
};

/**
 * The values of SGL Descriptor Sub Type (Base 2.4, 4.3.2, Figure 118, page 162, PDF page 188)
 */
enum xnvme_spec_base_transport_sgl_data_block_descriptor_sglid_sgldst {
	XNVME_SPEC_BASE_TRANSPORT_SGL_DATA_BLOCK_DESCRIPTOR_SGLID_SGLDST_0H_2H_3H_4H = 0x0, ///< 0h, 2h, 3h, 4h (Base 2.4, 4.3.2, Figure 118, page 162, PDF page 188)
	XNVME_SPEC_BASE_TRANSPORT_SGL_DATA_BLOCK_DESCRIPTOR_SGLID_SGLDST_ALL_OTHER_VALUES = 0x0, ///< All other values (Base 2.4, 4.3.2, Figure 118, page 162, PDF page 188)
	XNVME_SPEC_BASE_TRANSPORT_SGL_DATA_BLOCK_DESCRIPTOR_SGLID_SGLDST_0H_2H_3H = 0x1, ///< 0h, 2h, 3h (Base 2.4, 4.3.2, Figure 118, page 162, PDF page 188)
	XNVME_SPEC_BASE_TRANSPORT_SGL_DATA_BLOCK_DESCRIPTOR_SGLID_SGLDST_ALL_OTHER_VALUES_1H = 0x1, ///< All other values (Base 2.4, 4.3.2, Figure 118, page 162, PDF page 188)
};

/**
 * Controller List Format (Base 2.4, 4.6.1, Figure 139, page 172, PDF page 198)
 *
 * Followed, from byte 2, by as many Controller Identifier as the data says
 */
struct __attribute__((packed)) xnvme_spec_base_controller_list_format {
	uint16_t numcids; ///< Number of Controller Identifiers, bytes 1:0
};

/**
 * NQN Construction for Older NVM Subsystems (Base 2.4, 4.7.1.2, Figure 141, page 174, PDF page 200)
 */
struct __attribute__((packed)) xnvme_spec_base_nqn_construction_for_older_nvm_subsystems {
	uint8_t nss[27]; ///< NQN Starting String, bytes 26:0
	uint8_t vid[4]; ///< PCI Vendor ID, bytes 30:27
	uint8_t ssvid[4]; ///< PCI Subsystem Vendor ID, bytes 34:31
	uint8_t sn[20]; ///< Serial Number, bytes 54:35
	uint8_t mn[40]; ///< Model Number, bytes 94:55
	uint8_t pad[128]; ///< Padding, bytes 222:95
};

/**
 * Supported Log Pages (Base 2.4, 5.2.13.1.1, Figure 210, page 217, PDF page 243)
 */
struct __attribute__((packed)) xnvme_spec_base_log_supported_log_pages {
	uint32_t lids[256]; ///< Log Page Identifier Supported, 256 of 4 bytes of XNVME_SPEC_BASE_LID_SUPPORTED_AND_EFFECTS_*, bytes 1023:0
};

/** LID Specific Parameter (Base 2.4, 5.2.13.1.1, Figure 211, page 217, PDF page 243); bits 31:16 of 'LID Supported and Effects' */
#define XNVME_SPEC_BASE_LID_SUPPORTED_AND_EFFECTS_LIDSP 0xffff0000
/** Index Offset Supported (Base 2.4, 5.2.13.1.1, Figure 211, page 217, PDF page 243); bit 1 of 'LID Supported and Effects' */
#define XNVME_SPEC_BASE_LID_SUPPORTED_AND_EFFECTS_IOS 0x2
/** LID Supported (Base 2.4, 5.2.13.1.1, Figure 211, page 217, PDF page 243); bit 0 of 'LID Supported and Effects' */
#define XNVME_SPEC_BASE_LID_SUPPORTED_AND_EFFECTS_LSUPP 0x1

/**
 * Error Information Log Entry (Base 2.4, 5.2.13.1.2, Figure 212, page 218, PDF page 244)
 */
struct __attribute__((packed)) xnvme_spec_base_error_information_log_entry {
	uint64_t ecnt; ///< Error Count, bytes 7:0
	uint16_t sqid; ///< Submission Queue ID, bytes 9:8
	uint16_t cid; ///< Command ID, bytes 11:10
	uint16_t sts; ///< Status Info, bytes 13:12, see XNVME_SPEC_BASE_ERROR_INFORMATION_LOG_ENTRY_STS_*
	uint16_t pel; ///< Parameter Error Location, bytes 15:14, see XNVME_SPEC_BASE_ERROR_INFORMATION_LOG_ENTRY_PEL_*
	uint64_t lba; ///< Logical Block Address, bytes 23:16
	uint32_t nsid; ///< Namespace Identifier, bytes 27:24
	uint8_t vsia; ///< Vendor Specific Information Available, byte 28
	uint8_t trtype; ///< Transport Type, byte 29
	uint8_t csi; ///< Command Set Identifier, byte 30
	uint8_t opc; ///< Opcode, byte 31
	uint64_t csinfo; ///< Command Specific Information, bytes 39:32
	uint16_t ttsi; ///< Transport Type Specific Information, bytes 41:40
	uint8_t rsvd42[21]; ///< Reserved, bytes 62:42
	uint8_t lpver; ///< Log Page Version, byte 63
};

/** Status (Base 2.4, 5.2.13.1.2, Figure 212, page 218, PDF page 244); bits 15:1 of 'sts' */
#define XNVME_SPEC_BASE_ERROR_INFORMATION_LOG_ENTRY_STS_STATUS 0xfffe
/** Phase Tag (Base 2.4, 5.2.13.1.2, Figure 212, page 218, PDF page 244); bit 0 of 'sts' */
#define XNVME_SPEC_BASE_ERROR_INFORMATION_LOG_ENTRY_STS_P 0x1
/** Bit Location (Base 2.4, 5.2.13.1.2, Figure 212, page 218, PDF page 244); bits 10:8 of 'pel' */
#define XNVME_SPEC_BASE_ERROR_INFORMATION_LOG_ENTRY_PEL_BITLOC 0x700
/** Byte Location (Base 2.4, 5.2.13.1.2, Figure 212, page 218, PDF page 244); bits 7:0 of 'pel' */
#define XNVME_SPEC_BASE_ERROR_INFORMATION_LOG_ENTRY_PEL_BYTLOC 0xff

/**
 * SMART / Health Information (Base 2.4, 5.2.13.1.3, Figure 213, page 220, PDF page 247)
 */
struct __attribute__((packed)) xnvme_spec_base_log_smart_health_information {
	uint8_t cw; ///< Critical Warning, byte 0, see XNVME_SPEC_BASE_LOG_SMART_HEALTH_INFORMATION_CW_*
	uint16_t ctemp; ///< Composite Temperature, bytes 2:1
	uint8_t avsp; ///< Available Spare, byte 3
	uint8_t avspt; ///< Available Spare Threshold, byte 4
	uint8_t pused; ///< Percentage Used, byte 5
	uint8_t egcws; ///< Endurance Group Critical Warning Summary, byte 6, see XNVME_SPEC_BASE_LOG_SMART_HEALTH_INFORMATION_EGCWS_*
	uint8_t infw; ///< Informative Warning, byte 7, see XNVME_SPEC_BASE_LOG_SMART_HEALTH_INFORMATION_INFW_*
	uint8_t rsvd8[24]; ///< Reserved, bytes 31:8
	uint8_t dur[16]; ///< Data Units Read, bytes 47:32
	uint8_t duw[16]; ///< Data Units Written, bytes 63:48
	uint8_t hrc[16]; ///< Host Read Commands, bytes 79:64
	uint8_t hwc[16]; ///< Host Write Commands, bytes 95:80
	uint8_t cbt[16]; ///< Controller Busy Time, bytes 111:96
	uint8_t pwrc[16]; ///< Power Cycles, bytes 127:112
	uint8_t poh[16]; ///< Power On Hours, bytes 143:128
	uint8_t upl[16]; ///< Unexpected Power Losses, bytes 159:144
	uint8_t mdie[16]; ///< Media and Data Integrity Errors, bytes 175:160
	uint8_t neile[16]; ///< Number of Error Information Log Entries, bytes 191:176
	uint32_t wctt; ///< Warning Composite Temperature Time, bytes 195:192
	uint32_t cctt; ///< Critical Composite Temperature Time, bytes 199:196
	uint16_t tsen[8]; ///< Temperature Sensor, 8 of 2 bytes of XNVME_SPEC_BASE_TEMPERATURE_SENSOR_*, bytes 215:200
	uint32_t tmt1tc[2]; ///< Thermal Management Temperature Transition Count, 2 of 4 bytes, bytes 223:216
	uint32_t tttmt[2]; ///< Total Time For Thermal Management Temperature, 2 of 4 bytes, bytes 231:224
	uint64_t olec; ///< Operational Lifetime Energy Consumed, bytes 239:232
	uint32_t ipm; ///< Interval Power Measurement, bytes 243:240, see XNVME_SPEC_BASE_LOG_SMART_HEALTH_INFORMATION_IPM_*
	uint8_t rsvd244[268]; ///< Reserved, bytes 511:244
};

/** Indeterminate Personality State (Base 2.4, 5.2.13.1.3, Figure 213, page 220, PDF page 247); bit 6 of 'cw' */
#define XNVME_SPEC_BASE_LOG_SMART_HEALTH_INFORMATION_CW_IPS 0x40
/** Persistent Memory Region Read-Only (Base 2.4, 5.2.13.1.3, Figure 213, page 220, PDF page 247); bit 5 of 'cw' */
#define XNVME_SPEC_BASE_LOG_SMART_HEALTH_INFORMATION_CW_PMRRO 0x20
/** Volatile Memory Backup Failed (Base 2.4, 5.2.13.1.3, Figure 213, page 220, PDF page 247); bit 4 of 'cw' */
#define XNVME_SPEC_BASE_LOG_SMART_HEALTH_INFORMATION_CW_VMBF 0x10
/** All Media Read-Only (Base 2.4, 5.2.13.1.3, Figure 213, page 220, PDF page 247); bit 3 of 'cw' */
#define XNVME_SPEC_BASE_LOG_SMART_HEALTH_INFORMATION_CW_AMRO 0x8
/** NVM Subsystem Degraded Reliability (Base 2.4, 5.2.13.1.3, Figure 213, page 220, PDF page 247); bit 2 of 'cw' */
#define XNVME_SPEC_BASE_LOG_SMART_HEALTH_INFORMATION_CW_NDR 0x4
/** Temperature Threshold Condition (Base 2.4, 5.2.13.1.3, Figure 213, page 220, PDF page 247); bit 1 of 'cw' */
#define XNVME_SPEC_BASE_LOG_SMART_HEALTH_INFORMATION_CW_TTC 0x2
/** Available Spare Capacity Below Threshold (Base 2.4, 5.2.13.1.3, Figure 213, page 220, PDF page 247); bit 0 of 'cw' */
#define XNVME_SPEC_BASE_LOG_SMART_HEALTH_INFORMATION_CW_ASCBT 0x1
/** Endurance Group Read-Only (Base 2.4, 5.2.13.1.3, Figure 213, page 220, PDF page 247); bit 3 of 'egcws' */
#define XNVME_SPEC_BASE_LOG_SMART_HEALTH_INFORMATION_EGCWS_EGRO 0x8
/** Endurance Group Degraded Reliability (Base 2.4, 5.2.13.1.3, Figure 213, page 220, PDF page 247); bit 2 of 'egcws' */
#define XNVME_SPEC_BASE_LOG_SMART_HEALTH_INFORMATION_EGCWS_EGDR 0x4
/** Endurance Group Available Spare Capacity Below Threshold (Base 2.4, 5.2.13.1.3, Figure 213, page 220, PDF page 247); bit 0 of 'egcws' */
#define XNVME_SPEC_BASE_LOG_SMART_HEALTH_INFORMATION_EGCWS_EGASCBT 0x1
/** Voltage Log Threshold Warning (Base 2.4, 5.2.13.1.3, Figure 213, page 220, PDF page 247); bit 0 of 'infw' */
#define XNVME_SPEC_BASE_LOG_SMART_HEALTH_INFORMATION_INFW_VLTHW 0x1
/** Temperature Sensor Temperature (Base 2.4, 5.2.13.1.3, Figure 214, page 224, PDF page 251); bits 15:0 of 'Temperature Sensor' */
#define XNVME_SPEC_BASE_TEMPERATURE_SENSOR_TST 0xffff
/** Power Measurement Type (Base 2.4, 5.2.13.1.3, Figure 213, page 220, PDF page 247); bits 23:20 of 'ipm' */
#define XNVME_SPEC_BASE_LOG_SMART_HEALTH_INFORMATION_IPM_PMT 0xf00000
/** Interval Power Measurement Scale (Base 2.4, 5.2.13.1.3, Figure 213, page 220, PDF page 247); bits 17:16 of 'ipm' */
#define XNVME_SPEC_BASE_LOG_SMART_HEALTH_INFORMATION_IPM_IPMS 0x30000
/** Interval Power Measurement Value (Base 2.4, 5.2.13.1.3, Figure 213, page 220, PDF page 247); bits 15:0 of 'ipm' */
#define XNVME_SPEC_BASE_LOG_SMART_HEALTH_INFORMATION_IPM_IPMV 0xffff

/**
 * The values of Interval Power Measurement Scale (Base 2.4, 5.2.13.1.3, Figure 213, page 220, PDF page 247)
 */
enum xnvme_spec_base_log_smart_health_information_ipm_ipms {
	XNVME_SPEC_BASE_LOG_SMART_HEALTH_INFORMATION_IPM_IPMS_0_0001_W = 0x1, ///< 0.0001 W (Base 2.4, 5.2.13.1.3, Figure 213, page 220, PDF page 247)
	XNVME_SPEC_BASE_LOG_SMART_HEALTH_INFORMATION_IPM_IPMS_0_01_W = 0x2, ///< 0.01 W (Base 2.4, 5.2.13.1.3, Figure 213, page 220, PDF page 247)
};

/**
 * Firmware Slot Information (Base 2.4, 5.2.13.1.4, Figure 215, page 225, PDF page 252)
 */
struct __attribute__((packed)) xnvme_spec_base_log_firmware_slot_information {
	uint8_t afi; ///< Active Firmware Info, byte 0, see XNVME_SPEC_BASE_LOG_FIRMWARE_SLOT_INFORMATION_AFI_*
	uint8_t rsvd1[7]; ///< Reserved, bytes 7:1
	uint64_t frs[7]; ///< Firmware Revision for Slot, 7 of 8 bytes, bytes 63:8
	uint8_t rsvd64[448]; ///< Reserved, bytes 511:64
};

/** Next Active Firmware Slot (Base 2.4, 5.2.13.1.4, Figure 215, page 225, PDF page 252); bits 6:4 of 'afi' */
#define XNVME_SPEC_BASE_LOG_FIRMWARE_SLOT_INFORMATION_AFI_NAFS 0x70
/** Current Active Firmware Slot (Base 2.4, 5.2.13.1.4, Figure 215, page 225, PDF page 252); bits 2:0 of 'afi' */
#define XNVME_SPEC_BASE_LOG_FIRMWARE_SLOT_INFORMATION_AFI_CAFS 0x7

/**
 * Commands Supported and Effects (Base 2.4, 5.2.13.1.6, Figure 216, page 226, PDF page 253)
 */
struct __attribute__((packed)) xnvme_spec_base_log_commands_supported_and_effects {
	uint32_t acs[256]; ///< Admin Command Supported, 256 of 4 bytes of XNVME_SPEC_BASE_COMMANDS_SUPPORTED_AND_EFFECTS_*, bytes 1023:0
	uint32_t iocs[256]; ///< I/O Command Supported, 256 of 4 bytes of XNVME_SPEC_BASE_COMMANDS_SUPPORTED_AND_EFFECTS_*, bytes 2047:1024
	uint8_t rsvd2048[2048]; ///< Reserved, bytes 4095:2048
};

/** Command Scope (Base 2.4, 5.2.13.1.6, Figure 217, page 227, PDF page 254); bits 31:20 of 'Commands Supported and Effects' */
#define XNVME_SPEC_BASE_COMMANDS_SUPPORTED_AND_EFFECTS_CSP 0xfff00000
/** UUID Selection Supported (Base 2.4, 5.2.13.1.6, Figure 217, page 227, PDF page 254); bit 19 of 'Commands Supported and Effects' */
#define XNVME_SPEC_BASE_COMMANDS_SUPPORTED_AND_EFFECTS_USS 0x80000
/** Command Submission and Execution (Base 2.4, 5.2.13.1.6, Figure 217, page 227, PDF page 254); bits 18:16 of 'Commands Supported and Effects' */
#define XNVME_SPEC_BASE_COMMANDS_SUPPORTED_AND_EFFECTS_CSE 0x70000
/** Command Submission and Execution Relaxations (Base 2.4, 5.2.13.1.6, Figure 217, page 227, PDF page 254); bits 15:14 of 'Commands Supported and Effects' */
#define XNVME_SPEC_BASE_COMMANDS_SUPPORTED_AND_EFFECTS_CSER 0xc000
/** Controller Capability Change (Base 2.4, 5.2.13.1.6, Figure 217, page 227, PDF page 254); bit 4 of 'Commands Supported and Effects' */
#define XNVME_SPEC_BASE_COMMANDS_SUPPORTED_AND_EFFECTS_CCC 0x10
/** Namespace Inventory Change (Base 2.4, 5.2.13.1.6, Figure 217, page 227, PDF page 254); bit 3 of 'Commands Supported and Effects' */
#define XNVME_SPEC_BASE_COMMANDS_SUPPORTED_AND_EFFECTS_NIC 0x8
/** Namespace Capability Change (Base 2.4, 5.2.13.1.6, Figure 217, page 227, PDF page 254); bit 2 of 'Commands Supported and Effects' */
#define XNVME_SPEC_BASE_COMMANDS_SUPPORTED_AND_EFFECTS_NCC 0x4
/** Logical Block Content Change (Base 2.4, 5.2.13.1.6, Figure 217, page 227, PDF page 254); bit 1 of 'Commands Supported and Effects' */
#define XNVME_SPEC_BASE_COMMANDS_SUPPORTED_AND_EFFECTS_LBCC 0x2
/** Command Supported (Base 2.4, 5.2.13.1.6, Figure 217, page 227, PDF page 254); bit 0 of 'Commands Supported and Effects' */
#define XNVME_SPEC_BASE_COMMANDS_SUPPORTED_AND_EFFECTS_CSUPP 0x1

/**
 * Self-test Result (Base 2.4, 5.2.13.1.7, Figure 219, page 230, PDF page 257)
 */
struct __attribute__((packed)) xnvme_spec_base_self_test_result {
	uint8_t dsts; ///< Device Self-test Status, byte 0, see XNVME_SPEC_BASE_SELF_TEST_RESULT_DSTS_*
	uint8_t segn; ///< Segment Number, byte 1
	uint8_t vdinfo; ///< Valid Diagnostic Information, byte 2, see XNVME_SPEC_BASE_SELF_TEST_RESULT_VDINFO_*
	uint8_t rsvd3; ///< Reserved, byte 3
	uint64_t poh; ///< Power On Hours, bytes 11:4
	uint32_t nsid; ///< Namespace Identifier, bytes 15:12
	uint64_t flba; ///< Failing LBA, bytes 23:16
	uint8_t stct; ///< Status Code Type, byte 24, see XNVME_SPEC_BASE_SELF_TEST_RESULT_STCT_*
	uint8_t stc; ///< Status Code, byte 25
	uint16_t vs; ///< Vendor Specific, bytes 27:26
};

/** Self-test Code (Base 2.4, 5.2.13.1.7, Figure 219, page 230, PDF page 257); bits 7:4 of 'dsts' */
#define XNVME_SPEC_BASE_SELF_TEST_RESULT_DSTS_DSTC 0xf0
/** Device Self-test Result (Base 2.4, 5.2.13.1.7, Figure 219, page 230, PDF page 257); bits 3:0 of 'dsts' */
#define XNVME_SPEC_BASE_SELF_TEST_RESULT_DSTS_DSTR 0xf
/** SC Valid (Base 2.4, 5.2.13.1.7, Figure 219, page 230, PDF page 257); bit 3 of 'vdinfo' */
#define XNVME_SPEC_BASE_SELF_TEST_RESULT_VDINFO_SCVLD 0x8
/** SCT Valid (Base 2.4, 5.2.13.1.7, Figure 219, page 230, PDF page 257); bit 2 of 'vdinfo' */
#define XNVME_SPEC_BASE_SELF_TEST_RESULT_VDINFO_SCTVLD 0x4
/** FLBA Valid (Base 2.4, 5.2.13.1.7, Figure 219, page 230, PDF page 257); bit 1 of 'vdinfo' */
#define XNVME_SPEC_BASE_SELF_TEST_RESULT_VDINFO_FVLD 0x2
/** NSID Valid (Base 2.4, 5.2.13.1.7, Figure 219, page 230, PDF page 257); bit 0 of 'vdinfo' */
#define XNVME_SPEC_BASE_SELF_TEST_RESULT_VDINFO_NSIDVLD 0x1
/** Additional Status Code Info (Base 2.4, 5.2.13.1.7, Figure 219, page 230, PDF page 257); bits 2:0 of 'stct' */
#define XNVME_SPEC_BASE_SELF_TEST_RESULT_STCT_ASTCI 0x7

/**
 * The values of Device Self-test Result (Base 2.4, 5.2.13.1.7, Figure 219, page 230, PDF page 257)
 */
enum xnvme_spec_base_self_test_result_dsts_dstr {
	XNVME_SPEC_BASE_SELF_TEST_RESULT_DSTS_DSTR_ENTRY_IS_EMPTY = 0xf, ///< Entry is empty (Base 2.4, 5.2.13.1.7, Figure 219, page 230, PDF page 257)
};

/**
 * Device Self-test (Base 2.4, 5.2.13.1.7, Figure 218, page 229, PDF page 256)
 */
struct __attribute__((packed)) xnvme_spec_base_log_device_self_test {
	uint8_t cdsto; ///< Current Device Self-Test Operation, byte 0, see XNVME_SPEC_BASE_LOG_DEVICE_SELF_TEST_CDSTO_*
	uint8_t cdstc; ///< Current Device Self-Test Completion, byte 1, see XNVME_SPEC_BASE_LOG_DEVICE_SELF_TEST_CDSTC_*
	uint16_t rsvd2; ///< Reserved, bytes 3:2
	struct xnvme_spec_base_self_test_result rds[20]; ///< Result Data Structure, 20 of 28 bytes, bytes 563:4
};

/** Device Self-test Operation Status (Base 2.4, 5.2.13.1.7, Figure 218, page 229, PDF page 256); bits 3:0 of 'cdsto' */
#define XNVME_SPEC_BASE_LOG_DEVICE_SELF_TEST_CDSTO_DSTOS 0xf
/** Device Self-test Completion Status (Base 2.4, 5.2.13.1.7, Figure 218, page 229, PDF page 256); bits 6:0 of 'cdstc' */
#define XNVME_SPEC_BASE_LOG_DEVICE_SELF_TEST_CDSTC_DSTCS 0x7f

/**
 * Telemetry Host-Initiated (Base 2.4, 5.2.13.1.8, Figure 221, page 233, PDF page 260)
 *
 * Followed, from byte 512, by as many Telemetry Host-Initiated Data Block as the data says
 */
struct __attribute__((packed)) xnvme_spec_base_log_telemetry_host_initiated {
	uint8_t lid; ///< Log Page Identifier, byte 0
	uint32_t rsvd1; ///< Reserved, bytes 4:1
	uint8_t ieee[3]; ///< IEEE OUI Identifier, bytes 7:5
	uint16_t thda1lb[3]; ///< Telemetry Host-Initiated Data Area Last Block, 3 of 2 bytes, bytes 13:8
	uint16_t rsvd14; ///< Reserved, bytes 15:14
	uint32_t thda4lb; ///< Telemetry Host-Initiated Data Area 4 Last Block, bytes 19:16
	uint8_t rsvd20[360]; ///< Reserved, bytes 379:20
	uint8_t ths; ///< Telemetry Host-Initiated Scope, byte 380
	uint8_t thdgn; ///< Telemetry Host-Initiated Data Generation Number, byte 381
	uint8_t tcda; ///< Telemetry Controller-Initiated Data Available, byte 382
	uint8_t tcdgn; ///< Telemetry Controller-Initiated Data Generation Number, byte 383
	uint8_t rid[128]; ///< Reason Identifier, bytes 511:384
};

/**
 * The values of Telemetry Host-Initiated Scope (Base 2.4, 5.2.13.1.8, Figure 221, page 233, PDF page 260)
 */
enum xnvme_spec_base_log_telemetry_host_initiated_ths {
	XNVME_SPEC_BASE_LOG_TELEMETRY_HOST_INITIATED_THS_NOT_REPORTED = 0x0, ///< Not reported (Base 2.4, 5.2.13.1.8, Figure 221, page 233, PDF page 260)
	XNVME_SPEC_BASE_LOG_TELEMETRY_HOST_INITIATED_THS_CONTROLLER = 0x1, ///< Controller (Base 2.4, 5.2.13.1.8, Figure 221, page 233, PDF page 260)
	XNVME_SPEC_BASE_LOG_TELEMETRY_HOST_INITIATED_THS_NVM_SUBSYSTEM = 0x2, ///< NVM subsystem (Base 2.4, 5.2.13.1.8, Figure 221, page 233, PDF page 260)
};

/**
 * Telemetry Controller-Initiated (Base 2.4, 5.2.13.1.9, Figure 223, page 235, PDF page 262)
 *
 * Followed, from byte 512, by as many Telemetry Controller-Initiated Data Block as the data says
 */
struct __attribute__((packed)) xnvme_spec_base_log_telemetry_controller_initiated {
	uint8_t lid; ///< Log Page Identifier, byte 0
	uint32_t rsvd1; ///< Reserved, bytes 4:1
	uint8_t ieee[3]; ///< IEEE OUI Identifier, bytes 7:5
	uint16_t tcda1lb[3]; ///< Telemetry Controller-Initiated Data Area Last Block, 3 of 2 bytes, bytes 13:8
	uint16_t rsvd14; ///< Reserved, bytes 15:14
	uint32_t tcda4lb; ///< Telemetry Controller-Initiated Data Area 4 Last Block, bytes 19:16
	uint8_t rsvd20[361]; ///< Reserved, bytes 380:20
	uint8_t tcs; ///< Telemetry Controller-Initiated Scope, byte 381
	uint8_t tcda; ///< Telemetry Controller-Initiated Data Available, byte 382
	uint8_t tcdgn; ///< Telemetry Controller-Initiated Data Generation Number, byte 383
	uint8_t rid[128]; ///< Reason Identifier, bytes 511:384
};

/**
 * The values of Telemetry Controller-Initiated Scope (Base 2.4, 5.2.13.1.9, Figure 223, page 235, PDF page 262)
 */
enum xnvme_spec_base_log_telemetry_controller_initiated_tcs {
	XNVME_SPEC_BASE_LOG_TELEMETRY_CONTROLLER_INITIATED_TCS_NOT_REPORTED = 0x0, ///< Not reported (Base 2.4, 5.2.13.1.9, Figure 223, page 235, PDF page 262)
	XNVME_SPEC_BASE_LOG_TELEMETRY_CONTROLLER_INITIATED_TCS_CONTROLLER = 0x1, ///< Controller (Base 2.4, 5.2.13.1.9, Figure 223, page 235, PDF page 262)
	XNVME_SPEC_BASE_LOG_TELEMETRY_CONTROLLER_INITIATED_TCS_NVM_SUBSYSTEM = 0x2, ///< NVM subsystem (Base 2.4, 5.2.13.1.9, Figure 223, page 235, PDF page 262)
};

/**
 * Endurance Group Information (Base 2.4, 5.2.13.1.10, Figure 225, page 237, PDF page 264)
 */
struct __attribute__((packed)) xnvme_spec_base_log_endurance_group_information {
	uint8_t egcw; ///< Endurance Group Critical Warning, byte 0, see XNVME_SPEC_BASE_LOG_ENDURANCE_GROUP_INFORMATION_EGCW_*
	uint8_t egfeat; ///< Endurance Group Features, byte 1, see XNVME_SPEC_BASE_LOG_ENDURANCE_GROUP_INFORMATION_EGFEAT_*
	uint8_t rsvd2; ///< Reserved, byte 2
	uint8_t avsp; ///< Available Spare, byte 3
	uint8_t avspt; ///< Available Spare Threshold, byte 4
	uint8_t pused; ///< Percentage Used, byte 5
	uint16_t did; ///< Domain Identifier, bytes 7:6
	uint8_t rsvd8[24]; ///< Reserved, bytes 31:8
	uint8_t ee[16]; ///< Endurance Estimate, bytes 47:32
	uint8_t dur[16]; ///< Data Units Read, bytes 63:48
	uint8_t duw[16]; ///< Data Units Written, bytes 79:64
	uint8_t muw[16]; ///< Media Units Written, bytes 95:80
	uint8_t hrc[16]; ///< Host Read Commands, bytes 111:96
	uint8_t hwc[16]; ///< Host Write Commands, bytes 127:112
	uint8_t mdie[16]; ///< Media and Data Integrity Errors, bytes 143:128
	uint8_t neile[16]; ///< Number of Error Information Log Entries, bytes 159:144
	uint8_t tegcap[16]; ///< Total Endurance Group Capacity, bytes 175:160
	uint8_t uegcap[16]; ///< Unallocated Endurance Group Capacity, bytes 191:176
	uint8_t rsvd192[320]; ///< Reserved, bytes 511:192
};

/** Endurance Group Read-Only (Base 2.4, 5.2.13.1.10, Figure 225, page 237, PDF page 264); bit 3 of 'egcw' */
#define XNVME_SPEC_BASE_LOG_ENDURANCE_GROUP_INFORMATION_EGCW_EGRO 0x8
/** Endurance Group Degraded Reliability (Base 2.4, 5.2.13.1.10, Figure 225, page 237, PDF page 264); bit 2 of 'egcw' */
#define XNVME_SPEC_BASE_LOG_ENDURANCE_GROUP_INFORMATION_EGCW_EGDR 0x4
/** Endurance Group Available Spare Below (Base 2.4, 5.2.13.1.10, Figure 225, page 237, PDF page 264); bit 0 of 'egcw' */
#define XNVME_SPEC_BASE_LOG_ENDURANCE_GROUP_INFORMATION_EGCW_EGASB 0x1
/** Endurance Group Media (Base 2.4, 5.2.13.1.10, Figure 225, page 237, PDF page 264); bit 0 of 'egfeat' */
#define XNVME_SPEC_BASE_LOG_ENDURANCE_GROUP_INFORMATION_EGFEAT_EGRMEDIA 0x1

/**
 * Predictable Latency Per NVM Set (Base 2.4, 5.2.13.1.11, Figure 227, page 239, PDF page 266)
 */
struct __attribute__((packed)) xnvme_spec_base_log_predictable_latency_per_nvm_set {
	uint8_t stsnvms; ///< Status, byte 0, see XNVME_SPEC_BASE_LOG_PREDICTABLE_LATENCY_PER_NVM_SET_STSNVMS_*
	uint8_t rsvd1; ///< Reserved, byte 1
	uint16_t etyp; ///< Event Type, bytes 3:2, see XNVME_SPEC_BASE_LOG_PREDICTABLE_LATENCY_PER_NVM_SET_ETYP_*
	uint8_t rsvd4[28]; ///< Reserved, bytes 31:4
	uint64_t dtwrt; ///< DTWIN Reads Typical, bytes 39:32
	uint64_t dtwwt; ///< DTWIN Writes Typical, bytes 47:40
	uint64_t dtwtm; ///< DTWIN Time Maximum, bytes 55:48
	uint64_t ntwtmh; ///< NDWIN Time Minimum High, bytes 63:56
	uint64_t ntwtml; ///< NDWIN Time Minimum Low, bytes 71:64
	uint8_t rsvd72[56]; ///< Reserved, bytes 127:72
	uint64_t dtwre; ///< DTWIN Reads Estimate, bytes 135:128
	uint64_t dtwwe; ///< DTWIN Writes Estimate, bytes 143:136
	uint64_t dtwte; ///< DTWIN Time Estimate, bytes 151:144
	uint8_t rsvd152[360]; ///< Reserved, bytes 511:152
};

/** Predictable Latency Mode Window (Base 2.4, 5.2.13.1.11, Figure 227, page 239, PDF page 266); bits 2:0 of 'stsnvms' */
#define XNVME_SPEC_BASE_LOG_PREDICTABLE_LATENCY_PER_NVM_SET_STSNVMS_PLMW 0x7
/** Deterministic Excursion Autonomous Transition (Base 2.4, 5.2.13.1.11, Figure 227, page 239, PDF page 266); bit 15 of 'etyp' */
#define XNVME_SPEC_BASE_LOG_PREDICTABLE_LATENCY_PER_NVM_SET_ETYP_DEAT 0x8000
/** Max Value Exceeded Autonomous Transition (Base 2.4, 5.2.13.1.11, Figure 227, page 239, PDF page 266); bit 14 of 'etyp' */
#define XNVME_SPEC_BASE_LOG_PREDICTABLE_LATENCY_PER_NVM_SET_ETYP_MVEAT 0x4000
/** DTWIN Time Warning (Base 2.4, 5.2.13.1.11, Figure 227, page 239, PDF page 266); bit 2 of 'etyp' */
#define XNVME_SPEC_BASE_LOG_PREDICTABLE_LATENCY_PER_NVM_SET_ETYP_DTTW 0x4
/** DTWIN Writes Warning (Base 2.4, 5.2.13.1.11, Figure 227, page 239, PDF page 266); bit 1 of 'etyp' */
#define XNVME_SPEC_BASE_LOG_PREDICTABLE_LATENCY_PER_NVM_SET_ETYP_DTWW 0x2
/** DTWIN Reads Warning (Base 2.4, 5.2.13.1.11, Figure 227, page 239, PDF page 266); bit 0 of 'etyp' */
#define XNVME_SPEC_BASE_LOG_PREDICTABLE_LATENCY_PER_NVM_SET_ETYP_DTRW 0x1

/**
 * The values of Predictable Latency Mode Window (Base 2.4, 5.2.13.1.11, Figure 227, page 239, PDF page 266)
 */
enum xnvme_spec_base_log_predictable_latency_per_nvm_set_stsnvms_plmw {
	XNVME_SPEC_BASE_LOG_PREDICTABLE_LATENCY_PER_NVM_SET_STSNVMS_PLMW_NOT_USED = 0x0, ///< Not used (Base 2.4, 5.2.13.1.11, Figure 227, page 239, PDF page 266)
	XNVME_SPEC_BASE_LOG_PREDICTABLE_LATENCY_PER_NVM_SET_STSNVMS_PLMW_DETERMINISTIC_WINDOW = 0x1, ///< Deterministic Window (Base 2.4, 5.2.13.1.11, Figure 227, page 239, PDF page 266)
	XNVME_SPEC_BASE_LOG_PREDICTABLE_LATENCY_PER_NVM_SET_STSNVMS_PLMW_NON_DETERMINISTIC_WINDOW = 0x2, ///< Non-Deterministic Window (Base 2.4, 5.2.13.1.11, Figure 227, page 239, PDF page 266)
};

/**
 * Predictable Latency Event Aggregate (Base 2.4, 5.2.13.1.12, Figure 228, page 240, PDF page 267)
 *
 * Followed, from byte 8, by as many Entry as the data says
 */
struct __attribute__((packed)) xnvme_spec_base_log_predictable_latency_event_aggregate {
	uint64_t nument; ///< Number of Entries, bytes 7:0
};

/**
 * Asymmetric Namespace Access (Base 2.4, 5.2.13.1.13, Figure 230, page 241, PDF page 268)
 *
 * Followed, from byte 16, by as many ANA Group Descriptor as the data says
 *
 * Not members:
 * ANA Group Descriptor 1: at an offset the data gives, in 'ana_group_descriptor'
 * ANA Group Descriptor NAGD-1: at an offset the data gives, in 'ana_group_descriptor'
 */
struct __attribute__((packed)) xnvme_spec_base_log_asymmetric_namespace_access {
	uint64_t chgc; ///< Change Count, bytes 7:0
	uint16_t nagd; ///< Number of ANA Group Descriptors, bytes 9:8
	uint8_t rsvd10[6]; ///< Reserved, bytes 15:10
};

/**
 * ANA Group Descriptor format (Base 2.4, 5.2.13.1.13, Figure 231, page 242, PDF page 269)
 *
 * Followed, from byte 32, by as many Namespace Identifier as the data says
 */
struct __attribute__((packed)) xnvme_spec_base_ana_group_descriptor_format {
	uint32_t agid; ///< ANA Group ID, bytes 3:0
	uint32_t nnv; ///< Number of NSID Values, bytes 7:4
	uint64_t chgc; ///< Change Count, bytes 15:8
	uint8_t anasa; ///< Asymmetric Namespace Access State Attributes, byte 16, see XNVME_SPEC_BASE_ANA_GROUP_DESCRIPTOR_FORMAT_ANASA_*
	uint8_t rsvd17[15]; ///< Reserved, bytes 31:17
};

/** Asymmetric Namespace Access State (Base 2.4, 5.2.13.1.13, Figure 231, page 242, PDF page 269); bits 3:0 of 'anasa' */
#define XNVME_SPEC_BASE_ANA_GROUP_DESCRIPTOR_FORMAT_ANASA_ANAS 0xf

/**
 * The values of Asymmetric Namespace Access State (Base 2.4, 5.2.13.1.13, Figure 231, page 242, PDF page 269)
 */
enum xnvme_spec_base_ana_group_descriptor_format_anasa_anas {
	XNVME_SPEC_BASE_ANA_GROUP_DESCRIPTOR_FORMAT_ANASA_ANAS_ANA_OPTIMIZED_STATE = 0x1, ///< ANA Optimized state (Base 2.4, 5.2.13.1.13, Figure 231, page 242, PDF page 269)
	XNVME_SPEC_BASE_ANA_GROUP_DESCRIPTOR_FORMAT_ANASA_ANAS_ANA_NON_OPTIMIZED_STATE = 0x2, ///< ANA Non-Optimized state (Base 2.4, 5.2.13.1.13, Figure 231, page 242, PDF page 269)
	XNVME_SPEC_BASE_ANA_GROUP_DESCRIPTOR_FORMAT_ANASA_ANAS_ANA_INACCESSIBLE_STATE = 0x3, ///< ANA Inaccessible state (Base 2.4, 5.2.13.1.13, Figure 231, page 242, PDF page 269)
	XNVME_SPEC_BASE_ANA_GROUP_DESCRIPTOR_FORMAT_ANASA_ANAS_ANA_PERSISTENT_LOSS_STATE = 0x4, ///< ANA Persistent Loss state (Base 2.4, 5.2.13.1.13, Figure 231, page 242, PDF page 269)
	XNVME_SPEC_BASE_ANA_GROUP_DESCRIPTOR_FORMAT_ANASA_ANAS_ANA_CHANGE_STATE = 0xf, ///< ANA Change state (Base 2.4, 5.2.13.1.13, Figure 231, page 242, PDF page 269)
};

/**
 * Persistent Event (Base 2.4, 5.2.13.1.14, Figure 233, page 246, PDF page 273)
 *
 * Followed, from byte 512, by as many Persistent Event as the data says
 *
 * Not members:
 * Persistent Event N: at an offset the data gives, in 'persistent_event'
 */
struct __attribute__((packed)) xnvme_spec_base_log_persistent_event {
	uint8_t lid; ///< Log Page Identifier, byte 0
	uint8_t rsvd1[3]; ///< Reserved, bytes 3:1
	uint32_t tnev; ///< Total Number of Events, bytes 7:4
	uint64_t tll; ///< Total Log Length, bytes 15:8
	uint8_t lrev; ///< Log Revision, byte 16
	uint8_t rsvd17; ///< Reserved, byte 17
	uint16_t lhl; ///< Log Header Length, bytes 19:18
	uint64_t tstmp; ///< Timestamp, bytes 27:20
	uint8_t poh[16]; ///< Power on Hours, bytes 43:28
	uint64_t pwrcc; ///< Power Cycle Count, bytes 51:44
	uint16_t vid; ///< PCI Vendor ID, bytes 53:52
	uint16_t ssvid; ///< PCI Subsystem Vendor ID, bytes 55:54
	uint8_t sn[20]; ///< Serial Number, bytes 75:56
	uint8_t mn[40]; ///< Model Number, bytes 115:76
	uint8_t subnqn[256]; ///< NVM Subsystem NVMe Qualified Name, bytes 371:116
	uint16_t gnum; ///< Generation Number, bytes 373:372
	uint32_t rci; ///< Reporting Context Information, bytes 377:374, see XNVME_SPEC_BASE_LOG_PERSISTENT_EVENT_RCI_*
	uint8_t rsvd378[102]; ///< Reserved, bytes 479:378
	uint8_t seb[32]; ///< Supported Events Bitmap, bytes 511:480
};

/** Reporting Context Exists (Base 2.4, 5.2.13.1.14, Figure 233, page 246, PDF page 273); bit 18 of 'rci' */
#define XNVME_SPEC_BASE_LOG_PERSISTENT_EVENT_RCI_RCE 0x40000
/** Reporting Context Port Identifier Type (Base 2.4, 5.2.13.1.14, Figure 233, page 246, PDF page 273); bits 17:16 of 'rci' */
#define XNVME_SPEC_BASE_LOG_PERSISTENT_EVENT_RCI_RCPIT 0x30000
/** Reporting Context Port Identifier (Base 2.4, 5.2.13.1.14, Figure 233, page 246, PDF page 273); bits 15:0 of 'rci' */
#define XNVME_SPEC_BASE_LOG_PERSISTENT_EVENT_RCI_RCPID 0xffff

/**
 * Persistent Event Format (Base 2.4, 5.2.13.1.14, Figure 234, page 248, PDF page 275)
 *
 * Followed, from byte 24, by as many Vendor Specific Information as the data says
 *
 * Not members:
 * Event Data: at an offset the data gives, in 'vendor_specific_information'
 */
struct __attribute__((packed)) xnvme_spec_base_persistent_event_format {
	uint8_t et; ///< Event Type, byte 0
	uint8_t etr; ///< Event Type Revision, byte 1
	uint8_t ehl; ///< Event Header Length, byte 2
	uint8_t ehai; ///< Event Header Additional Information, byte 3, see XNVME_SPEC_BASE_PERSISTENT_EVENT_FORMAT_EHAI_*
	uint16_t cntlid; ///< Controller Identifier, bytes 5:4
	uint64_t etstp; ///< Event Timestamp, bytes 13:6
	uint16_t pelpid; ///< Port Identifier, bytes 15:14
	uint32_t rsvd16; ///< Reserved, bytes 19:16
	uint16_t vsil; ///< Vendor Specific Information Length, bytes 21:20
	uint16_t el; ///< Event Length, bytes 23:22
};

/** Port Identifier Type (Base 2.4, 5.2.13.1.14, Figure 234, page 248, PDF page 275); bits 1:0 of 'ehai' */
#define XNVME_SPEC_BASE_PERSISTENT_EVENT_FORMAT_EHAI_PIT 0x3

/**
 * SMART / Health Log Snapshot Event Data Format (Event Type 01h) (Base 2.4, 5.2.13.1.14.2.1, Figure 237, page 251, PDF page 278)
 */
struct __attribute__((packed)) xnvme_spec_base_smart_health_log_snapshot_event_data_format {
	uint8_t ed[512]; ///< Event Data, bytes 511:0
};

/**
 * Firmware Commit Event Data Format (Event Type 02h) (Base 2.4, 5.2.13.1.14.2.2, Figure 238, page 251, PDF page 278)
 */
struct __attribute__((packed)) xnvme_spec_base_firmware_commit_event_data_format {
	uint64_t ofr; ///< Old Firmware Revision, bytes 7:0
	uint64_t nfr; ///< New Firmware Revision, bytes 15:8
	uint8_t fca; ///< Firmware Commit Action, byte 16
	uint8_t fslt; ///< Firmware Slot, byte 17
	uint8_t stctfcc; ///< Status Code Type for Firmware Commit Command, byte 18
	uint8_t srfcc; ///< Status Returned for Firmware Commit Command, byte 19
	uint16_t vafcrc; ///< Vendor Assigned Firmware Commit Result Code, bytes 21:20
};

/**
 * Timestamp Change Event Format (Event Type 03h) (Base 2.4, 5.2.13.1.14.2.3, Figure 239, page 252, PDF page 279)
 */
struct __attribute__((packed)) xnvme_spec_base_timestamp_change_event_format {
	uint64_t ptstp; ///< Previous Timestamp, bytes 7:0
	uint64_t msr; ///< Milliseconds Since Reset, bytes 15:8
};

/**
 * Power-on or Reset Event (Event Type 04h) (Base 2.4, 5.2.13.1.14.2.4, Figure 240, page 252, PDF page 279)
 *
 * Followed, from byte 8, by as many Reset Information List as the data says
 */
struct __attribute__((packed)) xnvme_spec_base_power_on_or_reset_event {
	uint64_t frev; ///< Firmware Revision, bytes 7:0
};

/**
 * Controller Reset Information descriptor (Base 2.4, 5.2.13.1.14.2.4, Figure 241, page 252, PDF page 279)
 */
struct __attribute__((packed)) xnvme_spec_base_controller_reset_information_descriptor {
	uint16_t cntlid; ///< Controller ID, bytes 1:0
	uint8_t fa; ///< Firmware Activation, byte 2
	uint8_t oip; ///< Operation in Progress, byte 3, see XNVME_SPEC_BASE_CONTROLLER_RESET_INFORMATION_DESCRIPTOR_OIP_*
	uint8_t rsvd4[12]; ///< Reserved, bytes 15:4
	uint32_t cpwrc; ///< Controller Power Cycle, bytes 19:16
	uint64_t pom; ///< Power on milliseconds, bytes 27:20
	uint64_t ctstp; ///< Controller Timestamp, bytes 35:28
};

/** Reset During NVM Format (Base 2.4, 5.2.13.1.14.2.4, Figure 241, page 252, PDF page 279); bit 0 of 'oip' */
#define XNVME_SPEC_BASE_CONTROLLER_RESET_INFORMATION_DESCRIPTOR_OIP_RDNF 0x1

/**
 * NVM Subsystem Hardware Error Event Format (Event Type 05h) (Base 2.4, 5.2.13.1.14.2.5, Figure 242, page 253, PDF page 280)
 *
 * Followed, from byte 4, by as many Additional Hardware Error Information as the data says
 */
struct __attribute__((packed)) xnvme_spec_base_nvm_subsystem_hardware_error_event_format {
	uint16_t nsheec; ///< NVM Subsystem Hardware Error Event Code, bytes 1:0
	uint16_t rsvd2; ///< Reserved, bytes 3:2
};

/**
 * The values of NVM Subsystem Hardware Error Event Code (Base 2.4, 5.2.13.1.14.2.5, Figure 243, page 253, PDF page 280)
 */
enum xnvme_spec_base_nvm_subsystem_hardware_error_event_format_nsheec {
	XNVME_SPEC_BASE_NVM_SUBSYSTEM_HARDWARE_ERROR_EVENT_FORMAT_NSHEEC_PCIE_CORRECTABLE_ERROR = 0x1, ///< PCIe Correctable Error (Base 2.4, 5.2.13.1.14.2.5, Figure 243, page 253, PDF page 280)
	XNVME_SPEC_BASE_NVM_SUBSYSTEM_HARDWARE_ERROR_EVENT_FORMAT_NSHEEC_PCIE_UNCORRECTABLE_NON_FATAL_ERROR = 0x2, ///< PCIe Uncorrectable Non fatal Error (Base 2.4, 5.2.13.1.14.2.5, Figure 243, page 253, PDF page 280)
	XNVME_SPEC_BASE_NVM_SUBSYSTEM_HARDWARE_ERROR_EVENT_FORMAT_NSHEEC_PCIE_UNCORRECTABLE_FATAL_ERROR = 0x3, ///< PCIe Uncorrectable Fatal Error (Base 2.4, 5.2.13.1.14.2.5, Figure 243, page 253, PDF page 280)
	XNVME_SPEC_BASE_NVM_SUBSYSTEM_HARDWARE_ERROR_EVENT_FORMAT_NSHEEC_PCIE_LINK_STATUS_CHANGE = 0x4, ///< PCIe Link Status Change (Base 2.4, 5.2.13.1.14.2.5, Figure 243, page 253, PDF page 280)
	XNVME_SPEC_BASE_NVM_SUBSYSTEM_HARDWARE_ERROR_EVENT_FORMAT_NSHEEC_PCIE_LINK_NOT_ACTIVE = 0x5, ///< PCIe Link Not Active (Base 2.4, 5.2.13.1.14.2.5, Figure 243, page 253, PDF page 280)
	XNVME_SPEC_BASE_NVM_SUBSYSTEM_HARDWARE_ERROR_EVENT_FORMAT_NSHEEC_CRITICAL_WARNING_CONDITION = 0x6, ///< Critical Warning Condition (Base 2.4, 5.2.13.1.14.2.5, Figure 243, page 253, PDF page 280)
	XNVME_SPEC_BASE_NVM_SUBSYSTEM_HARDWARE_ERROR_EVENT_FORMAT_NSHEEC_ENDURANCE_GROUP_CRITICAL_WARNING_CONDITION = 0x7, ///< Endurance Group Critical Warning Condition (Base 2.4, 5.2.13.1.14.2.5, Figure 243, page 253, PDF page 280)
	XNVME_SPEC_BASE_NVM_SUBSYSTEM_HARDWARE_ERROR_EVENT_FORMAT_NSHEEC_UNEXPECTED_POWER_LOSS = 0x8, ///< Unexpected Power Loss (Base 2.4, 5.2.13.1.14.2.5, Figure 243, page 253, PDF page 280)
	XNVME_SPEC_BASE_NVM_SUBSYSTEM_HARDWARE_ERROR_EVENT_FORMAT_NSHEEC_CONTROLLER_FATAL_STATUS = 0x9, ///< Controller Fatal Status (Base 2.4, 5.2.13.1.14.2.5, Figure 243, page 253, PDF page 280)
	XNVME_SPEC_BASE_NVM_SUBSYSTEM_HARDWARE_ERROR_EVENT_FORMAT_NSHEEC_MEDIA_AND_DATA_INTEGRITY_STATUS = 0xa, ///< Media and Data Integrity Status (Base 2.4, 5.2.13.1.14.2.5, Figure 243, page 253, PDF page 280)
	XNVME_SPEC_BASE_NVM_SUBSYSTEM_HARDWARE_ERROR_EVENT_FORMAT_NSHEEC_CONTROLLER_READY_TIMEOUT_EXCEEDED = 0xb, ///< Controller Ready Timeout Exceeded (Base 2.4, 5.2.13.1.14.2.5, Figure 243, page 253, PDF page 280)
};

/**
 * Additional Hardware Error Information for Unexpected Power Loss Errors (Base 2.4, 5.2.13.1.14.2.5, Figure 244, page 255, PDF page 282)
 */
struct __attribute__((packed)) xnvme_spec_base_additional_hardware_error_information_for_unexpected_power_loss_errors {
	uint8_t upl[16]; ///< Unexpected Power Loss, bytes 15:0
	uint8_t upli; ///< Unexpected Power Loss Information, byte 16, see XNVME_SPEC_BASE_ADDITIONAL_HARDWARE_ERROR_INFORMATION_FOR_UNEXPECTED_POWER_LOSS_ERRORS_UPLI_*
};

/** Unexpected Power Loss Due to Out-Of-Band Activity (Base 2.4, 5.2.13.1.14.2.5, Figure 244, page 255, PDF page 282); bit 0 of 'upli' */
#define XNVME_SPEC_BASE_ADDITIONAL_HARDWARE_ERROR_INFORMATION_FOR_UNEXPECTED_POWER_LOSS_ERRORS_UPLI_UPLOA 0x1

/**
 * Additional Hardware Error Information for correctable and uncorrectable PCIe errors (Base 2.4, 5.2.13.1.14.2.5, Figure 245, page 255, PDF page 282)
 */
struct __attribute__((packed)) xnvme_spec_base_additional_hardware_error_information_for_correctable_and_uncorrectable_pcie_errors {
	uint16_t pciedsr; ///< PCIe Device Status Register, bytes 1:0
	uint8_t pcieas; ///< PCIe AER Support, byte 2, see XNVME_SPEC_BASE_ADDITIONAL_HARDWARE_ERROR_INFORMATION_FOR_CORRECTABLE_AND_UNCORRECTABLE_PCIE_ERRORS_PCIEAS_*
	uint8_t rsvd3[13]; ///< Reserved, bytes 15:3
	uint8_t pcieaes[16]; ///< PCIe AER Error Status, bytes 31:16
	uint8_t pcieaem[16]; ///< PCIe AER Error Mask, bytes 47:32
	uint8_t pcieahlr[16]; ///< PCIe AER Header Log Register, bytes 63:48
	uint8_t pcieatplr[16]; ///< PCIe AER TLP Prefix Log Register, bytes 79:64
};

/** PCIe AER Supported (Base 2.4, 5.2.13.1.14.2.5, Figure 245, page 255, PDF page 282); bit 0 of 'pcieas' */
#define XNVME_SPEC_BASE_ADDITIONAL_HARDWARE_ERROR_INFORMATION_FOR_CORRECTABLE_AND_UNCORRECTABLE_PCIE_ERRORS_PCIEAS_PCIEAERS 0x1

/**
 * Additional Hardware Error Information for Controller Ready Timeout Exceeded errors (Base 2.4, 5.2.13.1.14.2.5, Figure 246, page 257, PDF page 284)
 */
struct __attribute__((packed)) xnvme_spec_base_additional_hardware_error_information_for_controller_ready_timeout_exceeded_errors {
	uint8_t cst; ///< Controller State, byte 0, see XNVME_SPEC_BASE_ADDITIONAL_HARDWARE_ERROR_INFORMATION_FOR_CONTROLLER_READY_TIMEOUT_EXCEEDED_ERRORS_CST_*
	uint8_t rsvd1[3]; ///< Reserved, bytes 3:1
};

/** Controller Not Ready (Base 2.4, 5.2.13.1.14.2.5, Figure 246, page 257, PDF page 284); bit 3 of 'cst' */
#define XNVME_SPEC_BASE_ADDITIONAL_HARDWARE_ERROR_INFORMATION_FOR_CONTROLLER_READY_TIMEOUT_EXCEEDED_ERRORS_CST_CNR 0x8
/** Admin Command Media Not Ready (Base 2.4, 5.2.13.1.14.2.5, Figure 246, page 257, PDF page 284); bit 2 of 'cst' */
#define XNVME_SPEC_BASE_ADDITIONAL_HARDWARE_ERROR_INFORMATION_FOR_CONTROLLER_READY_TIMEOUT_EXCEEDED_ERRORS_CST_ACMNR 0x4
/** Namespace Not Ready (Base 2.4, 5.2.13.1.14.2.5, Figure 246, page 257, PDF page 284); bit 1 of 'cst' */
#define XNVME_SPEC_BASE_ADDITIONAL_HARDWARE_ERROR_INFORMATION_FOR_CONTROLLER_READY_TIMEOUT_EXCEEDED_ERRORS_CST_NNR 0x2
/** Controller Ready Independent of Media Enable (Base 2.4, 5.2.13.1.14.2.5, Figure 246, page 257, PDF page 284); bit 0 of 'cst' */
#define XNVME_SPEC_BASE_ADDITIONAL_HARDWARE_ERROR_INFORMATION_FOR_CONTROLLER_READY_TIMEOUT_EXCEEDED_ERRORS_CST_CRIME 0x1

/**
 * Change Namespace Event Data Format (Event Type 06h) (Base 2.4, 5.2.13.1.14.2.6, Figure 247, page 257, PDF page 284)
 */
struct __attribute__((packed)) xnvme_spec_base_change_namespace_event_data_format {
	uint32_t nmcdw10; ///< Namespace Management CDW10, bytes 3:0
	uint32_t rsvd4; ///< Reserved, bytes 7:4
	uint64_t nsze; ///< Namespace Size, bytes 15:8
	uint64_t rsvd16; ///< Reserved, bytes 23:16
	uint64_t ncap; ///< Namespace Capacity, bytes 31:24
	uint8_t flbas; ///< Formatted LBA Size, byte 32
	uint8_t dps; ///< End-to-end Data Protection Type Settings, byte 33
	uint8_t nmic; ///< Namespace Multi-path I/O and Namespace Sharing Capabilities, byte 34
	uint8_t rsvd35; ///< Reserved, byte 35
	uint32_t anagrpid; ///< ANA Group Identifier, bytes 39:36
	uint16_t nvmsetid; ///< NVM Set Identifier, bytes 41:40
	uint16_t endgid; ///< Endurance Group Identifier, bytes 43:42
	uint32_t nsid; ///< Namespace ID, bytes 47:44
};

/**
 * Format NVM Start Event Data Format (Event Type 07h) (Base 2.4, 5.2.13.1.14.2.7, Figure 248, page 259, PDF page 286)
 */
struct __attribute__((packed)) xnvme_spec_base_format_nvm_start_event_data_format {
	uint32_t nsid; ///< Namespace Identifier, bytes 3:0
	uint8_t fna; ///< Format NVM Attributes, byte 4
	uint8_t rsvd5[3]; ///< Reserved, bytes 7:5
	uint32_t fmcdw10; ///< Format NVM CDW10, bytes 11:8
};

/**
 * Format NVM Completion Event Data Format (Event Type 08h) (Base 2.4, 5.2.13.1.14.2.8, Figure 249, page 259, PDF page 286)
 */
struct __attribute__((packed)) xnvme_spec_base_format_nvm_completion_event_data_format {
	uint32_t nsid; ///< Namespace Identifier, bytes 3:0
	uint8_t sfpi; ///< Smallest Format Progress Indicator, byte 4
	uint8_t fnvms; ///< Format NVM Status, byte 5, see XNVME_SPEC_BASE_FORMAT_NVM_COMPLETION_EVENT_DATA_FORMAT_FNVMS_*
	uint16_t cinfo; ///< Completion Information, bytes 7:6
	uint16_t info; ///< Status Info, bytes 9:8, see XNVME_SPEC_BASE_FORMAT_NVM_COMPLETION_EVENT_DATA_FORMAT_INFO_*
	uint16_t rsvd10; ///< Reserved, bytes 11:10
};

/** Incomplete Format (Base 2.4, 5.2.13.1.14.2.8, Figure 249, page 259, PDF page 286); bit 1 of 'fnvms' */
#define XNVME_SPEC_BASE_FORMAT_NVM_COMPLETION_EVENT_DATA_FORMAT_FNVMS_INCPLTF 0x2
/** Format NVM Error (Base 2.4, 5.2.13.1.14.2.8, Figure 249, page 259, PDF page 286); bit 0 of 'fnvms' */
#define XNVME_SPEC_BASE_FORMAT_NVM_COMPLETION_EVENT_DATA_FORMAT_FNVMS_FNVME 0x1
/** Status (Base 2.4, 5.2.13.1.14.2.8, Figure 249, page 259, PDF page 286); bits 15:1 of 'info' */
#define XNVME_SPEC_BASE_FORMAT_NVM_COMPLETION_EVENT_DATA_FORMAT_INFO_STATUS 0xfffe
/** Phase Tag (Base 2.4, 5.2.13.1.14.2.8, Figure 249, page 259, PDF page 286); bit 0 of 'info' */
#define XNVME_SPEC_BASE_FORMAT_NVM_COMPLETION_EVENT_DATA_FORMAT_INFO_P 0x1

/**
 * Sanitize Start Event Data Format (Event Type 09h) (Base 2.4, 5.2.13.1.14.2.9, Figure 250, page 260, PDF page 287)
 */
struct __attribute__((packed)) xnvme_spec_base_sanitize_start_event_data_format {
	uint32_t sanicap; ///< Sanitize Capabilities, bytes 3:0
	uint32_t scdw10; ///< Sanitize CDW10, bytes 7:4
	uint32_t scdw11; ///< Sanitize CDW11, bytes 11:8
	uint32_t nsid; ///< Namespace Identifier, bytes 15:12
};

/**
 * Sanitize Completion Event Data Format (Event Type 0Ah) (Base 2.4, 5.2.13.1.14.2.10, Figure 251, page 260, PDF page 287)
 */
struct __attribute__((packed)) xnvme_spec_base_sanitize_completion_event_data_format {
	uint16_t sprog; ///< Sanitize Progress, bytes 1:0
	uint16_t sstat; ///< Sanitize Status, bytes 3:2
	uint16_t cinfo; ///< Completion Information, bytes 5:4
	uint16_t rsvd6; ///< Reserved, bytes 7:6
	uint32_t nsid; ///< Namespace Identifier, bytes 11:8
};

/**
 * Set Feature Event Data Format (Base 2.4, 5.2.13.1.14.2.11, Figure 253, page 263, PDF page 290)
 *
 * Followed, from byte 4, by as many Command Dwords as the data says
 *
 * Not members:
 * Memory Buffer: at an offset the data gives, in 'command_dwords'
 * Command Completion Dword 0: at an offset the data gives, in 'command_dwords'
 */
struct __attribute__((packed)) xnvme_spec_base_set_feature_event_data_format {
	uint32_t sfel; ///< Set Feature Event Layout, bytes 3:0, see XNVME_SPEC_BASE_SET_FEATURE_EVENT_DATA_FORMAT_SFEL_*
};

/** Memory Buffer Count (Base 2.4, 5.2.13.1.14.2.11, Figure 253, page 263, PDF page 290); bits 31:16 of 'sfel' */
#define XNVME_SPEC_BASE_SET_FEATURE_EVENT_DATA_FORMAT_SFEL_MBC 0xffff0000
/** Logged Command Completion Dword 0 (Base 2.4, 5.2.13.1.14.2.11, Figure 253, page 263, PDF page 290); bit 3 of 'sfel' */
#define XNVME_SPEC_BASE_SET_FEATURE_EVENT_DATA_FORMAT_SFEL_LCCDW0 0x8
/** Dword Count (Base 2.4, 5.2.13.1.14.2.11, Figure 253, page 263, PDF page 290); bits 2:0 of 'sfel' */
#define XNVME_SPEC_BASE_SET_FEATURE_EVENT_DATA_FORMAT_SFEL_DWC 0x7

/**
 * Telemetry Log Created Event Data Format (Event Type 0Ch) (Base 2.4, 5.2.13.1.14.2.12, Figure 254, page 263, PDF page 290)
 */
struct __attribute__((packed)) xnvme_spec_base_telemetry_log_created_event_data_format {
	uint8_t til[512]; ///< Telemetry Initiated Log, bytes 511:0
};

/**
 * Thermal Excursion Event Data Format (Event Type 0Dh) (Base 2.4, 5.2.13.1.14.2.13, Figure 255, page 264, PDF page 291)
 */
struct __attribute__((packed)) xnvme_spec_base_thermal_excursion_event_data_format {
	uint8_t otmp; ///< Over Temperature, byte 0
	uint8_t thresh; ///< Threshold, byte 1
};

/**
 * CDP Change Event Data Format (Event Type 0Fh) (Base 2.4, 5.2.13.1.14.2.15, Figure 256, page 266, PDF page 293)
 *
 * Followed, from byte 12, by as many Personality Event Data as the data says
 */
struct __attribute__((packed)) xnvme_spec_base_cdp_change_event_data_format {
	uint8_t ps; ///< Personality Status, byte 0, see XNVME_SPEC_BASE_CDP_CHANGE_EVENT_DATA_FORMAT_PS_*
	uint8_t perid; ///< Personality Identifier, byte 1
	uint8_t rsvd2[10]; ///< Reserved, bytes 11:2
};

/** CDP Requested Freeze State (Base 2.4, 5.2.13.1.14.2.15, Figure 256, page 266, PDF page 293); bit 1 of 'ps' */
#define XNVME_SPEC_BASE_CDP_CHANGE_EVENT_DATA_FORMAT_PS_CDPRFS 0x2
/** CDP Change Error (Base 2.4, 5.2.13.1.14.2.15, Figure 256, page 266, PDF page 293); bit 0 of 'ps' */
#define XNVME_SPEC_BASE_CDP_CHANGE_EVENT_DATA_FORMAT_PS_CDPCE 0x1

/**
 * Exported NVM Subsystem Change Event Data Format (Event Type 10h) (Base 2.4, 5.2.13.1.14.2.16, Figure 257, page 266, PDF page 293)
 */
struct __attribute__((packed)) xnvme_spec_base_exported_nvm_subsystem_change_event_data_format {
	uint16_t esubid; ///< Exported NVM Subsystem Identifier, bytes 1:0
	uint8_t otyp; ///< Operation Type, byte 2
	uint8_t rsvd3[5]; ///< Reserved, bytes 7:3
	uint32_t eeid; ///< Exported Entity Identifier, bytes 11:8
	uint32_t ueid; ///< Underlying Entity Identifier, bytes 15:12
};

/**
 * The values of Operation Type (Base 2.4, 5.2.13.1.14.2.16, Figure 257, page 266, PDF page 293)
 */
enum xnvme_spec_base_exported_nvm_subsystem_change_event_data_format_otyp {
	XNVME_SPEC_BASE_EXPORTED_NVM_SUBSYSTEM_CHANGE_EVENT_DATA_FORMAT_OTYP_ALLOWED_HOST_LIST_CHANGE = 0x8, ///< Allowed Host List Change (Base 2.4, 5.2.13.1.14.2.16, Figure 257, page 266, PDF page 293)
	XNVME_SPEC_BASE_EXPORTED_NVM_SUBSYSTEM_CHANGE_EVENT_DATA_FORMAT_OTYP_RESTRICTED_ACCESS_CHANGE = 0x9, ///< Restricted Access Change (Base 2.4, 5.2.13.1.14.2.16, Figure 257, page 266, PDF page 293)
};

/**
 * Vendor Specific Event Descriptor Format (Base 2.4, 5.2.13.1.14.2.17, Figure 259, page 268, PDF page 295)
 *
 * Followed, from byte 6, by as many Vendor Specific Event Data as the data says
 */
struct __attribute__((packed)) xnvme_spec_base_vendor_specific_event_descriptor_format {
	uint16_t vsec; ///< Vendor Specific Event Code, bytes 1:0
	uint8_t vsedt; ///< Vendor Specific Event Data Type, byte 2
	uint8_t uidx; ///< UUID Index, byte 3
	uint16_t vsedl; ///< Vendor Specific Event Data Length, bytes 5:4
};

/**
 * The values of Vendor Specific Event Data Type (Base 2.4, 5.2.13.1.14.2.17, Figure 260, page 268, PDF page 295)
 */
enum xnvme_spec_base_vendor_specific_event_descriptor_format_vsedt {
	XNVME_SPEC_BASE_VENDOR_SPECIFIC_EVENT_DESCRIPTOR_FORMAT_VSEDT_EVENT_NAME = 0x1, ///< Event Name (Base 2.4, 5.2.13.1.14.2.17, Figure 260, page 268, PDF page 295)
	XNVME_SPEC_BASE_VENDOR_SPECIFIC_EVENT_DESCRIPTOR_FORMAT_VSEDT_ASCII_STRING = 0x2, ///< ASCII String (Base 2.4, 5.2.13.1.14.2.17, Figure 260, page 268, PDF page 295)
	XNVME_SPEC_BASE_VENDOR_SPECIFIC_EVENT_DESCRIPTOR_FORMAT_VSEDT_BINARY = 0x3, ///< Binary (Base 2.4, 5.2.13.1.14.2.17, Figure 260, page 268, PDF page 295)
	XNVME_SPEC_BASE_VENDOR_SPECIFIC_EVENT_DESCRIPTOR_FORMAT_VSEDT_SIGNED_INTEGER = 0x4, ///< Signed Integer (Base 2.4, 5.2.13.1.14.2.17, Figure 260, page 268, PDF page 295)
};

/**
 * Endurance Group Event Aggregate (Base 2.4, 5.2.13.1.15, Figure 261, page 269, PDF page 296)
 *
 * Followed, from byte 8, by as many Entry as the data says
 */
struct __attribute__((packed)) xnvme_spec_base_log_endurance_group_event_aggregate {
	uint64_t nument; ///< Number of Entries, bytes 7:0
};

/**
 * Media Unit Status (Base 2.4, 5.2.13.1.16, Figure 263, page 270, PDF page 297)
 *
 * Followed, from byte 16, by as many Media Unit Status Descriptor as the data says
 *
 * Not members:
 * Media Unit Status Descriptor 1: at an offset the data gives, in 'media_unit_status_descriptor'
 * Media Unit Status Descriptor NMU-1: at an offset the data gives, in 'media_unit_status_descriptor'
 */
struct __attribute__((packed)) xnvme_spec_base_log_media_unit_status {
	uint16_t nmu; ///< Number of Media Unit Status Descriptors, bytes 1:0
	uint16_t cchans; ///< Number of Channels, bytes 3:2
	uint16_t selc; ///< Selected Configuration, bytes 5:4
	uint8_t rsvd6[10]; ///< Reserved, bytes 15:6
};

/**
 * Media Unit Status Descriptor (Base 2.4, 5.2.13.1.16, Figure 264, page 271, PDF page 298)
 *
 * Followed, from byte 14, by as many Reserved as the data says
 *
 * Not members:
 * Channel 0 Identifier: at an offset the data gives, in 'reserved'
 * Channel 1 Identifier: at an offset the data gives, in 'reserved'
 * Channel MUCS-1 Identifier: at an offset the data gives, in 'reserved'
 */
struct __attribute__((packed)) xnvme_spec_base_media_unit_status_descriptor {
	uint16_t muid; ///< Media Unit Identifier, bytes 1:0
	uint16_t did; ///< Domain Identifier, bytes 3:2
	uint16_t endgid; ///< Endurance Group Identifier, bytes 5:4
	uint16_t nvmsetid; ///< NVM Set Identifier, bytes 7:6
	uint16_t caf; ///< Capacity Adjustment Factor, bytes 9:8
	uint8_t avsp; ///< Available Spare, byte 10
	uint8_t pused; ///< Percentage Used, byte 11
	uint8_t mucs; ///< Number of Channels, byte 12
	uint8_t cio; ///< Channel Identifiers Offset, byte 13
};

/**
 * Supported Capacity Configuration List (Base 2.4, 5.2.13.1.17, Figure 265, page 272, PDF page 299)
 *
 * Followed, from byte 16, by as many Capacity Configuration Descriptor as the data says
 *
 * Not members:
 * Capacity Configuration 1 Descriptor: at an offset the data gives, in 'capacity_configuration_descriptor'
 * Capacity Configuration SCCN-1 Descriptor: at an offset the data gives, in 'capacity_configuration_descriptor'
 */
struct __attribute__((packed)) xnvme_spec_base_log_supported_capacity_configuration_list {
	uint8_t sccn; ///< Number of Supported Capacity Configurations, byte 0
	uint8_t rsvd1[15]; ///< Reserved, bytes 15:1
};

/**
 * Capacity Configuration Descriptor (Base 2.4, 5.2.13.1.17, Figure 266, page 272, PDF page 299)
 *
 * Followed, from byte 32, by as many Endurance Group Configuration Descriptor as the data says
 *
 * Not members:
 * Endurance Group Configuration 1 Descriptor: at an offset the data gives, in 'endurance_group_configuration_descriptor'
 * Endurance Group Configuration EGCN-1 Descriptor: at an offset the data gives, in 'endurance_group_configuration_descriptor'
 */
struct __attribute__((packed)) xnvme_spec_base_capacity_configuration_descriptor {
	uint16_t ccid; ///< Capacity Configuration Identifier, bytes 1:0
	uint16_t did; ///< Domain Identifier, bytes 3:2
	uint16_t egcn; ///< Number of Endurance Group Configuration Descriptors, bytes 5:4
	uint8_t rsvd6[26]; ///< Reserved, bytes 31:6
};

/**
 * Endurance Group Configuration Descriptor (Base 2.4, 5.2.13.1.17, Figure 267, page 273, PDF page 300)
 *
 * Followed, from byte 82, by as many NVM Set Identifier as the data says
 *
 * Not members:
 * Number of Channels: at an offset the data gives, in 'nvm_set_identifier'
 * Channel 0 Configuration Descriptor: at an offset the data gives, in 'nvm_set_identifier'
 * Channel 1 Configuration Descriptor: at an offset the data gives, in 'nvm_set_identifier'
 * Channel EGCHANS-1 Configuration Descriptor: at an offset the data gives, in 'nvm_set_identifier'
 */
struct __attribute__((packed)) xnvme_spec_base_endurance_group_configuration_descriptor {
	uint16_t endgid; ///< Endurance Group Identifier, bytes 1:0
	uint16_t cadjf; ///< Capacity Adjustment Factor, bytes 3:2
	uint8_t rsvd4[12]; ///< Reserved, bytes 15:4
	uint8_t tegcap[16]; ///< Total Endurance Group Capacity, bytes 31:16
	uint8_t segcap[16]; ///< Spare Endurance Group Capacity, bytes 47:32
	uint8_t ee[16]; ///< Endurance Estimate, bytes 63:48
	uint8_t rsvd64[16]; ///< Reserved, bytes 79:64
	uint16_t egsets; ///< Number of NVM Sets, bytes 81:80
};

/**
 * Media Unit Configuration Descriptor (Base 2.4, 5.2.13.1.17, Figure 269, page 275, PDF page 302)
 */
struct __attribute__((packed)) xnvme_spec_base_media_unit_configuration_descriptor {
	uint16_t muid; ///< Media Unit Identifier, bytes 1:0
	uint32_t rsvd2; ///< Reserved, bytes 5:2
	uint16_t mudl; ///< Media Unit Descriptor Length, bytes 7:6
};

/**
 * Channel Configuration Descriptor (Base 2.4, 5.2.13.1.17, Figure 268, page 274, PDF page 301)
 *
 * Followed, from byte 4, by as many Media Unit Configuration Descriptor as the data says, each a 'struct xnvme_spec_base_media_unit_configuration_descriptor'
 *
 * Not members:
 * Media Unit 1 Configuration Descriptor: at an offset the data gives, in 'media_unit_configuration_descriptor'
 * Media Unit CHMUS-1 Configuration Descriptor: at an offset the data gives, in 'media_unit_configuration_descriptor'
 */
struct __attribute__((packed)) xnvme_spec_base_channel_configuration_descriptor {
	uint16_t chid; ///< Channel Identifier, bytes 1:0
	uint16_t chmus; ///< Number of Channel Media Units, bytes 3:2
};

/**
 * Feature Identifiers Effects (Base 2.4, 5.2.13.1.18, Figure 270, page 275, PDF page 302)
 */
struct __attribute__((packed)) xnvme_spec_base_log_feature_identifiers_effects {
	uint32_t fis[256]; ///< Feature Identifier Supported, 256 of 4 bytes of XNVME_SPEC_BASE_FID_SUPPORTED_AND_EFFECTS_*, bytes 1023:0
};

/** FID Scope (Base 2.4, 5.2.13.1.18, Figure 271, page 276, PDF page 303); bits 31:20 of 'FID Supported and Effects' */
#define XNVME_SPEC_BASE_FID_SUPPORTED_AND_EFFECTS_FSP 0xfff00000
/** UUID Selection Supported (Base 2.4, 5.2.13.1.18, Figure 271, page 276, PDF page 303); bit 19 of 'FID Supported and Effects' */
#define XNVME_SPEC_BASE_FID_SUPPORTED_AND_EFFECTS_USS 0x80000
/** Controller Capability Change (Base 2.4, 5.2.13.1.18, Figure 271, page 276, PDF page 303); bit 4 of 'FID Supported and Effects' */
#define XNVME_SPEC_BASE_FID_SUPPORTED_AND_EFFECTS_CCC 0x10
/** Namespace Inventory Change (Base 2.4, 5.2.13.1.18, Figure 271, page 276, PDF page 303); bit 3 of 'FID Supported and Effects' */
#define XNVME_SPEC_BASE_FID_SUPPORTED_AND_EFFECTS_NIC 0x8
/** Namespace Capability Change (Base 2.4, 5.2.13.1.18, Figure 271, page 276, PDF page 303); bit 2 of 'FID Supported and Effects' */
#define XNVME_SPEC_BASE_FID_SUPPORTED_AND_EFFECTS_NCC 0x4
/** User Data Content Change (Base 2.4, 5.2.13.1.18, Figure 271, page 276, PDF page 303); bit 1 of 'FID Supported and Effects' */
#define XNVME_SPEC_BASE_FID_SUPPORTED_AND_EFFECTS_UDCC 0x2
/** FID Supported (Base 2.4, 5.2.13.1.18, Figure 271, page 276, PDF page 303); bit 0 of 'FID Supported and Effects' */
#define XNVME_SPEC_BASE_FID_SUPPORTED_AND_EFFECTS_FSUPP 0x1

/**
 * NVMe-MI Commands Supported and Effects (Base 2.4, 5.2.13.1.19, Figure 272, page 277, PDF page 304)
 */
struct __attribute__((packed)) xnvme_spec_base_log_nvme_mi_commands_supported_and_effects {
	uint32_t mics[256]; ///< Management Interface Command Supported, 256 of 4 bytes of XNVME_SPEC_BASE_NVME_MI_COMMANDS_SUPPORTED_AND_EFFECTS_*, bytes 1023:0
	uint8_t rsvd1024[3072]; ///< Reserved, bytes 4095:1024
};

/** Command Scope (Base 2.4, 5.2.13.1.19, Figure 273, page 278, PDF page 305); bits 31:20 of 'NVMe-MI Commands Supported and Effects' */
#define XNVME_SPEC_BASE_NVME_MI_COMMANDS_SUPPORTED_AND_EFFECTS_CSP 0xfff00000
/** Controller Capability Change (Base 2.4, 5.2.13.1.19, Figure 273, page 278, PDF page 305); bit 4 of 'NVMe-MI Commands Supported and Effects' */
#define XNVME_SPEC_BASE_NVME_MI_COMMANDS_SUPPORTED_AND_EFFECTS_CCC 0x10
/** Namespace Inventory Change (Base 2.4, 5.2.13.1.19, Figure 273, page 278, PDF page 305); bit 3 of 'NVMe-MI Commands Supported and Effects' */
#define XNVME_SPEC_BASE_NVME_MI_COMMANDS_SUPPORTED_AND_EFFECTS_NIC 0x8
/** Namespace Capability Change (Base 2.4, 5.2.13.1.19, Figure 273, page 278, PDF page 305); bit 2 of 'NVMe-MI Commands Supported and Effects' */
#define XNVME_SPEC_BASE_NVME_MI_COMMANDS_SUPPORTED_AND_EFFECTS_NCC 0x4
/** User Data Content Change (Base 2.4, 5.2.13.1.19, Figure 273, page 278, PDF page 305); bit 1 of 'NVMe-MI Commands Supported and Effects' */
#define XNVME_SPEC_BASE_NVME_MI_COMMANDS_SUPPORTED_AND_EFFECTS_UDCC 0x2
/** Command Supported (Base 2.4, 5.2.13.1.19, Figure 273, page 278, PDF page 305); bit 0 of 'NVMe-MI Commands Supported and Effects' */
#define XNVME_SPEC_BASE_NVME_MI_COMMANDS_SUPPORTED_AND_EFFECTS_CSUPP 0x1

/**
 * Command and Feature Lockdown (Base 2.4, 5.2.13.1.20, Figure 276, page 280, PDF page 307)
 *
 * Followed, from byte 4, by as many Command and Feature Identifier List as the data says
 *
 * Not members:
 * Reserved: at an offset the data gives, in 'command_and_feature_identifier_list'
 */
struct __attribute__((packed)) xnvme_spec_base_log_command_and_feature_lockdown {
	uint8_t cfila; ///< Command and Feature Identifier List Attributes, byte 0, see XNVME_SPEC_BASE_LOG_COMMAND_AND_FEATURE_LOCKDOWN_CFILA_*
	uint16_t rsvd1; ///< Reserved, bytes 2:1
	uint8_t lngth; ///< Length, byte 3
};

/** Contents Selected (Base 2.4, 5.2.13.1.20, Figure 276, page 280, PDF page 307); bits 5:4 of 'cfila' */
#define XNVME_SPEC_BASE_LOG_COMMAND_AND_FEATURE_LOCKDOWN_CFILA_CS 0x30
/** Scope Selected (Base 2.4, 5.2.13.1.20, Figure 276, page 280, PDF page 307); bits 3:0 of 'cfila' */
#define XNVME_SPEC_BASE_LOG_COMMAND_AND_FEATURE_LOCKDOWN_CFILA_SS 0xf

/**
 * Command and Feature Identifier Descriptor (Base 2.4, 5.2.13.1.20, Figure 278, page 282, PDF page 309)
 */
struct __attribute__((packed)) xnvme_spec_base_command_and_feature_identifier_descriptor {
	uint8_t cfi; ///< Command and Feature Identifier, byte 0
	uint8_t cfia; ///< Command and Feature Identifier Attributes, byte 1, see XNVME_SPEC_BASE_COMMAND_AND_FEATURE_IDENTIFIER_DESCRIPTOR_CFIA_*
};

/** All Controllers (Base 2.4, 5.2.13.1.20, Figure 278, page 282, PDF page 309); bit 0 of 'cfia' */
#define XNVME_SPEC_BASE_COMMAND_AND_FEATURE_IDENTIFIER_DESCRIPTOR_CFIA_ACNTL 0x1

/**
 * Command and Feature Lockdown Log Page - Enhanced (Base 2.4, 5.2.13.1.20, Figure 277, page 280, PDF page 308)
 *
 * Followed, from byte 16, by as many Command and Feature Identifier Descriptor as the data says, each a 'struct xnvme_spec_base_command_and_feature_identifier_descriptor'
 *
 * Not members:
 * Command and Feature Identifier Descriptor 1: at an offset the data gives, in 'command_and_feature_identifier_descriptor'
 * Command and Feature Identifier Descriptor NCFID-1: at an offset the data gives, in 'command_and_feature_identifier_descriptor'
 */
struct __attribute__((packed)) xnvme_spec_base_enhanced {
	uint8_t ver; ///< Version, byte 0
	uint8_t cfia; ///< Command and Feature Identifier Attributes, byte 1, see XNVME_SPEC_BASE_ENHANCED_CFIA_*
	uint16_t cntlid; ///< Controller Identifier, bytes 3:2
	uint32_t sze; ///< Size, bytes 7:4
	uint16_t ncfid; ///< Number of Command and Feature Identifier Descriptors, bytes 9:8
	uint16_t cfids; ///< Command and Feature Identifier Descriptors Size, bytes 11:10
	uint32_t rsvd12; ///< Reserved, bytes 15:12
};

/** Contents Selected (Base 2.4, 5.2.13.1.20, Figure 277, page 280, PDF page 308); bits 5:4 of 'cfia' */
#define XNVME_SPEC_BASE_ENHANCED_CFIA_CS 0x30
/** Scope Selected (Base 2.4, 5.2.13.1.20, Figure 277, page 280, PDF page 308); bits 3:0 of 'cfia' */
#define XNVME_SPEC_BASE_ENHANCED_CFIA_SS 0xf

/**
 * Boot Partition (Base 2.4, 5.2.13.1.21, Figure 280, page 282, PDF page 310)
 *
 * Followed, from byte 16, by as many Boot Partition Data as the data says
 */
struct __attribute__((packed)) xnvme_spec_base_log_boot_partition {
	uint8_t lid; ///< Log Page Identifier, byte 0
	uint8_t rsvd1[3]; ///< Reserved, bytes 3:1
	uint32_t bpinfo; ///< Boot Partition Information, bytes 7:4, see XNVME_SPEC_BASE_LOG_BOOT_PARTITION_BPINFO_*
	uint64_t rsvd8; ///< Reserved, bytes 15:8
};

/** Active Boot Partition ID (Base 2.4, 5.2.13.1.21, Figure 280, page 282, PDF page 310); bit 31 of 'bpinfo' */
#define XNVME_SPEC_BASE_LOG_BOOT_PARTITION_BPINFO_ABPID 0x80000000
/** Boot Partition Size (Base 2.4, 5.2.13.1.21, Figure 280, page 282, PDF page 310); bits 14:0 of 'bpinfo' */
#define XNVME_SPEC_BASE_LOG_BOOT_PARTITION_BPINFO_BPSZ 0x7fff

/**
 * Rotational Media Information (Base 2.4, 5.2.13.1.22, Figure 281, page 283, PDF page 310)
 */
struct __attribute__((packed)) xnvme_spec_base_log_rotational_media_information {
	uint16_t endgid; ///< Endurance Group Identifier, bytes 1:0
	uint16_t numa; ///< Number of Actuators, bytes 3:2
	uint16_t nrs; ///< Nominal Rotational Speed, bytes 5:4
	uint16_t rsvd6; ///< Reserved, bytes 7:6
	uint32_t spinc; ///< Spinup Count, bytes 11:8
	uint32_t fspinc; ///< Failed Spinup Count, bytes 15:12
	uint32_t ldc; ///< Load Count, bytes 19:16
	uint32_t fldc; ///< Failed Load Count, bytes 23:20
	uint8_t rsvd24[488]; ///< Reserved, bytes 511:24
};

/**
 * Dispersed Namespace Participating NVM Subsystems (Base 2.4, 5.2.13.1.23, Figure 282, page 284, PDF page 311)
 *
 * Followed, from byte 256, by as many Participating NVM Subsystem Entry as the data says
 */
struct __attribute__((packed)) xnvme_spec_base_log_dispersed_namespace_participating_nvm_subsystems {
	uint64_t genctr; ///< Generation Counter, bytes 7:0
	uint64_t numpsub; ///< Number of Participating NVM Subsystems, bytes 15:8
	uint8_t rsvd16[240]; ///< Reserved, bytes 255:16
};

/**
 * Management Address Descriptor (Base 2.4, 5.2.13.1.24, Figure 284, page 285, PDF page 312)
 */
struct __attribute__((packed)) xnvme_spec_base_management_address_descriptor {
	uint8_t mat; ///< Management Address Type, byte 0
	uint8_t rsvd1[3]; ///< Reserved, bytes 3:1
	uint8_t madrs[508]; ///< Management Address, bytes 511:4
};

/**
 * Management Address List - (Base 2.4, 5.2.13.1.24, Figure 283, page 285, PDF page 312)
 */
struct __attribute__((packed)) xnvme_spec_base_log_management_address_list {
	struct xnvme_spec_base_management_address_descriptor mad[8]; ///< Management Address Descriptor, 8 of 512 bytes, bytes 4095:0
};

/**
 * Reachability Groups (Base 2.4, 5.2.13.1.25, Figure 286, page 286, PDF page 313)
 *
 * Followed, from byte 16, by as many Reachability Group Descriptor as the data says
 *
 * Not members:
 * Reachability Group Descriptor 1: at an offset the data gives, in 'reachability_group_descriptor'
 * Reachability Group Descriptor NRGD-1: at an offset the data gives, in 'reachability_group_descriptor'
 */
struct __attribute__((packed)) xnvme_spec_base_log_reachability_groups {
	uint64_t chngc; ///< Change Count, bytes 7:0
	uint16_t nrgd; ///< Number of Reachability Group Descriptors, bytes 9:8
	uint8_t rsvd10[6]; ///< Reserved, bytes 15:10
};

/**
 * Reachability Group Descriptor format (Base 2.4, 5.2.13.1.25, Figure 287, page 287, PDF page 314)
 *
 * Followed, from byte 32, by as many Namespace Identifier as the data says
 */
struct __attribute__((packed)) xnvme_spec_base_reachability_group_descriptor_format {
	uint32_t rgid; ///< Reachability Group ID, bytes 3:0
	uint32_t nnid; ///< Number of NSID Values, bytes 7:4
	uint64_t chngc; ///< Change Count, bytes 15:8
	uint8_t rsvd16[16]; ///< Reserved, bytes 31:16
};

/**
 * Reachability Associations (Base 2.4, 5.2.13.1.26, Figure 289, page 288, PDF page 315)
 *
 * Followed, from byte 16, by as many Reachability Association Descriptor as the data says
 *
 * Not members:
 * Reachability Association Descriptor 1: at an offset the data gives, in 'reachability_association_descriptor'
 * Reachability Association Descriptor NRAD-1: at an offset the data gives, in 'reachability_association_descriptor'
 */
struct __attribute__((packed)) xnvme_spec_base_log_reachability_associations {
	uint64_t chngc; ///< Change Count, bytes 7:0
	uint16_t nrad; ///< Number of Reachability Association Descriptors, bytes 9:8
	uint8_t rsvd10[6]; ///< Reserved, bytes 15:10
};

/**
 * Reachability Association Descriptor format (Base 2.4, 5.2.13.1.26, Figure 290, page 288, PDF page 316)
 *
 * Followed, from byte 32, by as many Reachability Group Identifier as the data says
 */
struct __attribute__((packed)) xnvme_spec_base_reachability_association_descriptor_format {
	uint32_t rasid; ///< Reachability Association ID, bytes 3:0
	uint32_t nrid; ///< Number of RGID Values, bytes 7:4
	uint64_t chngc; ///< Change Count, bytes 15:8
	uint8_t rac; ///< Reachability Association Characteristics, byte 16
	uint8_t rsvd17[15]; ///< Reserved, bytes 31:17
};

/**
 * Device Personalities (Base 2.4, 5.2.13.1.28, Figure 291, page 290, PDF page 317)
 *
 * Followed, from byte 6, by as many Personality as the data says
 *
 * Not members:
 * Personality 1: at an offset the data gives, in 'personality'
 * Personality NUMP-1: at an offset the data gives, in 'personality'
 */
struct __attribute__((packed)) xnvme_spec_base_log_device_personalities {
	uint16_t nump; ///< Number of Personalities, bytes 1:0
	uint8_t cdplpv; ///< CDP Log Page Version, byte 2
	uint8_t dplphl; ///< Device Personalities Log Page Header Length, byte 3
	uint16_t cdplps; ///< Device Personalities Log Page Size, bytes 5:4
};

/**
 * Personality Properties (Base 2.4, 5.2.13.1.28, Figure 292, page 290, PDF page 317)
 */
struct __attribute__((packed)) xnvme_spec_base_personality_properties {
	uint8_t pps; ///< Personality Properties Size, byte 0
	uint8_t perid; ///< Personality Identifier, byte 1
	uint8_t mrstt; ///< Minimum Required Reset Type, byte 2
	uint8_t aus; ///< Authenticated Unfreeze Support, byte 3
	uint8_t personality_attributes; ///< Personality Attributes, byte 4, see XNVME_SPEC_BASE_PERSONALITY_PROPERTIES_PERSONALITY_ATTRIBUTES_*
};

/** Personality Settings Change User Data Effect (Base 2.4, 5.2.13.1.28, Figure 292, page 290, PDF page 317); bit 0 of 'personality_attributes' */
#define XNVME_SPEC_BASE_PERSONALITY_PROPERTIES_PERSONALITY_ATTRIBUTES_PSCUDE 0x1

/**
 * The values of Personality Identifier (Base 2.4, 5.2.30.1.25, Figure 512, page 483, PDF page 511)
 */
enum xnvme_spec_base_personality_properties_perid {
	XNVME_SPEC_BASE_PERSONALITY_PROPERTIES_PERID_MANUFACTURING_DEFAULT_PERSONALITY = 0x0, ///< Manufacturing Default Personality (Base 2.4, 5.2.30.1.25, Figure 512, page 483, PDF page 511)
	XNVME_SPEC_BASE_PERSONALITY_PROPERTIES_PERID_SECURITY_PERSONALITY = 0x1, ///< Security Personality (Base 2.4, 5.2.30.1.25, Figure 512, page 483, PDF page 511)
	XNVME_SPEC_BASE_PERSONALITY_PROPERTIES_PERID_LOCKDOWN_PERSISTENCE_PERSONALITY = 0x2, ///< Lockdown Persistence Personality (Base 2.4, 5.2.30.1.25, Figure 512, page 483, PDF page 511)
	XNVME_SPEC_BASE_PERSONALITY_PROPERTIES_PERID_ALL_PERSONALITIES = 0xff, ///< All Personalities (Base 2.4, 5.2.30.1.25, Figure 512, page 483, PDF page 511)
};

/**
 * The values of Minimum Required Reset Type (Base 2.4, 5.2.13.1.28, Figure 292, page 290, PDF page 317)
 */
enum xnvme_spec_base_personality_properties_mrstt {
	XNVME_SPEC_BASE_PERSONALITY_PROPERTIES_MRSTT_NO_RESET = 0x0, ///< No Reset (Base 2.4, 5.2.13.1.28, Figure 292, page 290, PDF page 317)
	XNVME_SPEC_BASE_PERSONALITY_PROPERTIES_MRSTT_CONTROLLER_LEVEL_RESET = 0x1, ///< Controller Level Reset (Base 2.4, 5.2.13.1.28, Figure 292, page 290, PDF page 317)
	XNVME_SPEC_BASE_PERSONALITY_PROPERTIES_MRSTT_LIMITED_CONTROLLER_LEVEL_RESET = 0x2, ///< Limited Controller Level Reset (Base 2.4, 5.2.13.1.28, Figure 292, page 290, PDF page 317)
	XNVME_SPEC_BASE_PERSONALITY_PROPERTIES_MRSTT_NVM_SUBSYSTEM_RESET = 0x3, ///< NVM Subsystem Reset (Base 2.4, 5.2.13.1.28, Figure 292, page 290, PDF page 317)
	XNVME_SPEC_BASE_PERSONALITY_PROPERTIES_MRSTT_POWER_CYCLE = 0x4, ///< Power Cycle (Base 2.4, 5.2.13.1.28, Figure 292, page 290, PDF page 317)
};

/**
 * FDP Configurations (Base 2.4, 5.2.13.1.29, Figure 293, page 291, PDF page 319)
 *
 * Followed, from byte 16, by as many FDP Configuration Descriptor as the data says
 *
 * Not members:
 * FDP Configuration Descriptor 1: at an offset the data gives, in 'fdp_configuration_descriptor'
 * FDP Configuration Descriptor NUMFDPC-1: at an offset the data gives, in 'fdp_configuration_descriptor'
 */
struct __attribute__((packed)) xnvme_spec_base_log_fdp_configurations {
	uint16_t numfdpc; ///< Number of FDP Configurations, bytes 1:0
	uint8_t ver; ///< Version, byte 2
	uint8_t rsvd3; ///< Reserved, byte 3
	uint32_t sze; ///< Size, bytes 7:4
	uint64_t rsvd8; ///< Reserved, bytes 15:8
};

/**
 * Reclaim Unit Handle Descriptor (Base 2.4, 5.2.13.1.29, Figure 295, page 293, PDF page 321)
 */
struct __attribute__((packed)) xnvme_spec_base_reclaim_unit_handle_descriptor {
	uint8_t ruht; ///< Reclaim Unit Handle Type, byte 0
	uint8_t rsvd1[3]; ///< Reserved, bytes 3:1
};

/**
 * The values of Reclaim Unit Handle Type (Base 2.4, 5.2.13.1.29, Figure 295, page 293, PDF page 321)
 */
enum xnvme_spec_base_reclaim_unit_handle_descriptor_ruht {
	XNVME_SPEC_BASE_RECLAIM_UNIT_HANDLE_DESCRIPTOR_RUHT_INITIALLY_ISOLATED = 0x1, ///< Initially Isolated (Base 2.4, 5.2.13.1.29, Figure 295, page 293, PDF page 321)
	XNVME_SPEC_BASE_RECLAIM_UNIT_HANDLE_DESCRIPTOR_RUHT_PERSISTENTLY_ISOLATED = 0x2, ///< Persistently Isolated (Base 2.4, 5.2.13.1.29, Figure 295, page 293, PDF page 321)
};

/**
 * FDP Configuration Descriptor (Base 2.4, 5.2.13.1.29, Figure 294, page 292, PDF page 319)
 *
 * Followed, from byte 64, by as many Reclaim Unit Handle Descriptor as the data says, each a 'struct xnvme_spec_base_reclaim_unit_handle_descriptor'
 *
 * Not members:
 * Vendor Specific: at an offset the data gives, in 'reclaim_unit_handle_descriptor'
 * Pad: at an offset the data gives, in 'reclaim_unit_handle_descriptor'
 */
struct __attribute__((packed)) xnvme_spec_base_fdp_configuration_descriptor {
	uint16_t dsze; ///< Descriptor Size, bytes 1:0
	uint8_t fdpa; ///< FDP Attributes, byte 2, see XNVME_SPEC_BASE_FDP_CONFIGURATION_DESCRIPTOR_FDPA_*
	uint8_t vss; ///< Vendor Specific Size, byte 3
	uint32_t nrg; ///< Number of Reclaim Groups, bytes 7:4
	uint16_t nruh; ///< Number of Reclaim Unit Handles, bytes 9:8
	uint16_t maxpids; ///< Max Placement Identifiers, bytes 11:10, see XNVME_SPEC_BASE_FDP_CONFIGURATION_DESCRIPTOR_MAXPIDS_*
	uint32_t nns; ///< Number of Namespaces Supported, bytes 15:12
	uint64_t runs; ///< Reclaim Unit Nominal Size, bytes 23:16
	uint32_t erutl; ///< Estimated Reclaim Unit Time Limit, bytes 27:24
	uint8_t rsvd28[36]; ///< Reserved, bytes 63:28
};

/** FDP Configuration Valid (Base 2.4, 5.2.13.1.29, Figure 294, page 292, PDF page 319); bit 7 of 'fdpa' */
#define XNVME_SPEC_BASE_FDP_CONFIGURATION_DESCRIPTOR_FDPA_FDPCV 0x80
/** FDP Volatile Write Cache (Base 2.4, 5.2.13.1.29, Figure 294, page 292, PDF page 319); bit 4 of 'fdpa' */
#define XNVME_SPEC_BASE_FDP_CONFIGURATION_DESCRIPTOR_FDPA_FDPVWC 0x10
/** Reclaim Group Identifier Format (Base 2.4, 5.2.13.1.29, Figure 294, page 292, PDF page 319); bits 3:0 of 'fdpa' */
#define XNVME_SPEC_BASE_FDP_CONFIGURATION_DESCRIPTOR_FDPA_RGIF 0xf
/** Number of Placement Identifiers (Base 2.4, 7.4.1.1, Figure 656, page 566, PDF page 596); bits 15:0 of 'maxpids' */
#define XNVME_SPEC_BASE_FDP_CONFIGURATION_DESCRIPTOR_MAXPIDS_NPID 0xffff

/**
 * Reclaim Unit Handle Usage Descriptor (Base 2.4, 5.2.13.1.30, Figure 299, page 295, PDF page 322)
 */
struct __attribute__((packed)) xnvme_spec_base_reclaim_unit_handle_usage_descriptor {
	uint8_t ruha; ///< Reclaim Unit Handle Attributes, byte 0
	uint8_t rsvd1[7]; ///< Reserved, bytes 7:1
};

/**
 * The values of Reclaim Unit Handle Attributes (Base 2.4, 5.2.13.1.30, Figure 299, page 295, PDF page 322)
 */
enum xnvme_spec_base_reclaim_unit_handle_usage_descriptor_ruha {
	XNVME_SPEC_BASE_RECLAIM_UNIT_HANDLE_USAGE_DESCRIPTOR_RUHA_HOST_SPECIFIED = 0x1, ///< Host Specified (Base 2.4, 5.2.13.1.30, Figure 299, page 295, PDF page 322)
	XNVME_SPEC_BASE_RECLAIM_UNIT_HANDLE_USAGE_DESCRIPTOR_RUHA_CONTROLLER_SPECIFIED = 0x2, ///< Controller Specified (Base 2.4, 5.2.13.1.30, Figure 299, page 295, PDF page 322)
};

/**
 * Reclaim Unit Handle Usage (Base 2.4, 5.2.13.1.30, Figure 298, page 294, PDF page 322)
 *
 * Followed, from byte 8, by as many Reclaim Unit Handle Usage Descriptor as the data says, each a 'struct xnvme_spec_base_reclaim_unit_handle_usage_descriptor'
 */
struct __attribute__((packed)) xnvme_spec_base_log_reclaim_unit_handle_usage {
	uint16_t nruh; ///< Number of Reclaim Unit Handles, bytes 1:0
	uint8_t rsvd2[6]; ///< Reserved, bytes 7:2
};

/**
 * FDP Statistics (Base 2.4, 5.2.13.1.31, Figure 300, page 295, PDF page 323)
 */
struct __attribute__((packed)) xnvme_spec_base_log_fdp_statistics {
	uint8_t hbmw[16]; ///< Host Bytes with Metadata Written, bytes 15:0
	uint8_t mbmw[16]; ///< Media Bytes with Metadata Written, bytes 31:16
	uint8_t mbe[16]; ///< Media Bytes Erased, bytes 47:32
	uint8_t rsvd48[16]; ///< Reserved, bytes 63:48
};

/**
 * FDP Event (Base 2.4, 5.2.13.1.32, Figure 303, page 297, PDF page 325)
 */
struct __attribute__((packed)) xnvme_spec_base_fdp_event {
	uint8_t etyp; ///< Event Type, byte 0
	uint8_t fdpef; ///< FDP Event Flags, byte 1, see XNVME_SPEC_BASE_FDP_EVENT_FDPEF_*
	uint16_t pid; ///< Placement Identifier, bytes 3:2
	uint64_t etmsp; ///< Event Timestamp, bytes 11:4
	uint32_t nsid; ///< Namespace Identifier, bytes 15:12
	uint8_t etsp[16]; ///< Event Type Specific, bytes 31:16
	uint16_t rgicd; ///< Reclaim Group Identifier, bytes 33:32
	uint16_t ruhid; ///< Reclaim Unit Handle Identifier, bytes 35:34
	uint32_t rsvd36; ///< Reserved, bytes 39:36
	uint8_t vs[24]; ///< Vendor Specific, bytes 63:40
};

/** Location Valid (Base 2.4, 5.2.13.1.32, Figure 303, page 297, PDF page 325); bit 2 of 'fdpef' */
#define XNVME_SPEC_BASE_FDP_EVENT_FDPEF_LV 0x4
/** NSID Valid (Base 2.4, 5.2.13.1.32, Figure 303, page 297, PDF page 325); bit 1 of 'fdpef' */
#define XNVME_SPEC_BASE_FDP_EVENT_FDPEF_NSIDV 0x2
/** Placement Identifier Valid (Base 2.4, 5.2.13.1.32, Figure 303, page 297, PDF page 325); bit 0 of 'fdpef' */
#define XNVME_SPEC_BASE_FDP_EVENT_FDPEF_PIV 0x1

/**
 * The values of Event Type (Base 2.4, 5.2.13.1.32, Figure 303, page 297, PDF page 325)
 */
enum xnvme_spec_base_fdp_event_etyp {
	XNVME_SPEC_BASE_FDP_EVENT_ETYP_INVALID_PLACEMENT_IDENTIFIER = 0x3, ///< Invalid Placement Identifier (Base 2.4, 5.2.13.1.32, Figure 303, page 297, PDF page 325)
	XNVME_SPEC_BASE_FDP_EVENT_ETYP_MEDIA_REALLOCATED = 0x80, ///< Media Reallocated (Base 2.4, 5.2.13.1.32, Figure 303, page 297, PDF page 325)
};

/**
 * FDP Events (Base 2.4, 5.2.13.1.32, Figure 302, page 297, PDF page 324)
 *
 * Followed, from byte 64, by as many FDP Event as the data says, each a 'struct xnvme_spec_base_fdp_event'
 */
struct __attribute__((packed)) xnvme_spec_base_log_fdp_events {
	uint32_t numfdpe; ///< Number of FDP Events, bytes 3:0
	uint8_t rsvd4[60]; ///< Reserved, bytes 63:4
};

/**
 * Manufacturer Default Configuration Status (Base 2.4, 5.2.13.1.33, Figure 304, page 299, PDF page 327)
 */
struct __attribute__((packed)) xnvme_spec_base_log_manufacturer_default_configuration_status {
	uint8_t mdcsv; ///< Manufacturer Default Configuration Status Version, byte 0
	uint8_t mdcs; ///< Manufacturer Default Configuration Status, byte 1, see XNVME_SPEC_BASE_LOG_MANUFACTURER_DEFAULT_CONFIGURATION_STATUS_MDCS_*
};

/** Default Capacity Configuration Status (Base 2.4, 5.2.13.1.33, Figure 304, page 299, PDF page 327); bit 2 of 'mdcs' */
#define XNVME_SPEC_BASE_LOG_MANUFACTURER_DEFAULT_CONFIGURATION_STATUS_MDCS_DCCS 0x4
/** Default Namespace Configuration Status (Base 2.4, 5.2.13.1.33, Figure 304, page 299, PDF page 327); bit 1 of 'mdcs' */
#define XNVME_SPEC_BASE_LOG_MANUFACTURER_DEFAULT_CONFIGURATION_STATUS_MDCS_DNCS 0x2
/** Default NVM Subsystem Configuration Status (Base 2.4, 5.2.13.1.33, Figure 304, page 299, PDF page 327); bit 0 of 'mdcs' */
#define XNVME_SPEC_BASE_LOG_MANUFACTURER_DEFAULT_CONFIGURATION_STATUS_MDCS_DSCS 0x1

/**
 * Power Histogram Descriptor (Base 2.4, 5.2.13.1.34, Figure 306, page 304, PDF page 332)
 */
struct __attribute__((packed)) xnvme_spec_base_power_histogram_descriptor {
	uint32_t phbc; ///< Power Histogram Bin Count, bytes 3:0
	uint32_t phblt; ///< Power Histogram Bin Lower Threshold, bytes 7:4, see XNVME_SPEC_BASE_POWER_HISTOGRAM_DESCRIPTOR_PHBLT_*
};

/** Power Scale (Base 2.4, 5.2.13.1.34, Figure 306, page 304, PDF page 332); bits 17:16 of 'phblt' */
#define XNVME_SPEC_BASE_POWER_HISTOGRAM_DESCRIPTOR_PHBLT_PWRS 0x30000
/** Power Value (Base 2.4, 5.2.13.1.34, Figure 306, page 304, PDF page 332); bits 15:0 of 'phblt' */
#define XNVME_SPEC_BASE_POWER_HISTOGRAM_DESCRIPTOR_PHBLT_PWRV 0xffff

/**
 * Power Measurement (Base 2.4, 5.2.13.1.34, Figure 305, page 301, PDF page 329)
 *
 * Followed, from byte 64, by as many Power Histogram Descriptor as the data says, each a 'struct xnvme_spec_base_power_histogram_descriptor'
 *
 * Not members:
 * Power Histogram Descriptor 1: at an offset the data gives, in 'power_histogram_descriptor'
 * Power Histogram Descriptor NPHD-1: at an offset the data gives, in 'power_histogram_descriptor'
 * Vendor Specific: at an offset the data gives, in 'power_histogram_descriptor'
 */
struct __attribute__((packed)) xnvme_spec_base_log_power_measurement {
	uint8_t ver; ///< Version, byte 0
	uint8_t pmgn; ///< Power Measurement Generation Number, byte 1
	uint16_t pma; ///< Power Measurement Attributes, bytes 3:2, see XNVME_SPEC_BASE_LOG_POWER_MEASUREMENT_PMA_*
	uint32_t sze; ///< Size, bytes 7:4
	uint32_t pmc; ///< Power Measurement Count, bytes 11:8
	uint16_t nphd; ///< Number of Power Histogram Descriptors, bytes 13:12
	uint16_t smtr; ///< Stop Measurement Time Remaining, bytes 15:14
	uint64_t smts; ///< Stop Measurement Timestamp, bytes 23:16
	uint16_t phds; ///< Power Histogram Descriptor Size, bytes 25:24
	uint16_t phbs; ///< Power Histogram Bin Size, bytes 27:26
	uint16_t nphds; ///< Number of Power Histogram Descriptors Supported, bytes 29:28
	uint16_t vss; ///< Vendor Specific Size, bytes 31:30
	uint32_t phdoc; ///< Power Histogram Descriptor Overflow Count, bytes 35:32
	uint32_t rsvd36; ///< Reserved, bytes 39:36
	uint32_t aipwr; ///< Average Interval Power, bytes 43:40, see XNVME_SPEC_BASE_LOG_POWER_MEASUREMENT_AIPWR_*
	uint32_t mipwr; ///< Maximum Interval Power, bytes 47:44, see XNVME_SPEC_BASE_LOG_POWER_MEASUREMENT_MIPWR_*
	uint64_t mipwrt; ///< Maximum Interval Power Timestamp, bytes 55:48
	uint8_t ipwrpe; ///< Interval Power Percent Error, byte 56
	uint8_t rsvd57[7]; ///< Reserved, bytes 63:57
};

/** Power Measurement Type (Base 2.4, 5.2.13.1.34, Figure 305, page 301, PDF page 329); bits 15:12 of 'pma' */
#define XNVME_SPEC_BASE_LOG_POWER_MEASUREMENT_PMA_PMT 0xf000
/** Power Histogram Descriptor Overflow (Base 2.4, 5.2.13.1.34, Figure 305, page 301, PDF page 329); bit 4 of 'pma' */
#define XNVME_SPEC_BASE_LOG_POWER_MEASUREMENT_PMA_PHDO 0x10
/** Maximum Interval Power Timestamp Support (Base 2.4, 5.2.13.1.34, Figure 305, page 301, PDF page 329); bit 3 of 'pma' */
#define XNVME_SPEC_BASE_LOG_POWER_MEASUREMENT_PMA_MIPWRTS 0x8
/** Estimated Power Flag (Base 2.4, 5.2.13.1.34, Figure 305, page 301, PDF page 329); bit 2 of 'pma' */
#define XNVME_SPEC_BASE_LOG_POWER_MEASUREMENT_PMA_EPF 0x4
/** Non-Contiguous Power Data Flag (Base 2.4, 5.2.13.1.34, Figure 305, page 301, PDF page 329); bit 1 of 'pma' */
#define XNVME_SPEC_BASE_LOG_POWER_MEASUREMENT_PMA_NCPDF 0x2
/** Power Measurement Enable (Base 2.4, 5.2.13.1.34, Figure 305, page 301, PDF page 329); bit 0 of 'pma' */
#define XNVME_SPEC_BASE_LOG_POWER_MEASUREMENT_PMA_PME 0x1
/** Average Interval Power Scale (Base 2.4, 5.2.13.1.34, Figure 305, page 301, PDF page 329); bits 17:16 of 'aipwr' */
#define XNVME_SPEC_BASE_LOG_POWER_MEASUREMENT_AIPWR_AIPWRS 0x30000
/** Average Interval Power Value (Base 2.4, 5.2.13.1.34, Figure 305, page 301, PDF page 329); bits 15:0 of 'aipwr' */
#define XNVME_SPEC_BASE_LOG_POWER_MEASUREMENT_AIPWR_AIPWRV 0xffff
/** Maximum Interval Power Scale (Base 2.4, 5.2.13.1.34, Figure 305, page 301, PDF page 329); bits 17:16 of 'mipwr' */
#define XNVME_SPEC_BASE_LOG_POWER_MEASUREMENT_MIPWR_MIPWRS 0x30000
/** Maximum Interval Power Value (Base 2.4, 5.2.13.1.34, Figure 305, page 301, PDF page 329); bits 15:0 of 'mipwr' */
#define XNVME_SPEC_BASE_LOG_POWER_MEASUREMENT_MIPWR_MIPWRV 0xffff

/**
 * Voltage Measurement (Base 2.4, 5.2.13.1.35, Figure 307, page 305, PDF page 333)
 *
 * Followed, from byte 64, by as many Interval Voltage Measurement Descriptor as the data says
 *
 * Not members:
 * Voltage Log Vendor Specific: at an offset the data gives, in 'interval_voltage_measurement_descriptor'
 */
struct __attribute__((packed)) xnvme_spec_base_log_voltage_measurement {
	uint8_t vmgn; ///< Voltage Measurement Generation Number, byte 0
	uint8_t vma; ///< Voltage Measurement Attributes, byte 1, see XNVME_SPEC_BASE_LOG_VOLTAGE_MEASUREMENT_VMA_*
	uint16_t vsi; ///< Voltage Sensor Info, bytes 3:2, see XNVME_SPEC_BASE_LOG_VOLTAGE_MEASUREMENT_VSI_*
	uint32_t vmlsz; ///< Voltage Measurement Log Size, bytes 7:4
	uint16_t nvmds; ///< Number of Voltage Measurement Descriptors Supported, bytes 9:8
	uint16_t nvmde; ///< Number of Voltage Measurement Descriptors Entries, bytes 11:10
	uint16_t svmtr; ///< Stop Voltage Measurement Time Remaining, bytes 13:12
	uint64_t svmts; ///< Stop Voltage Measurement Timestamp, bytes 21:14
	uint16_t cntlid; ///< Controller Identifier, bytes 23:22
	uint16_t vlvss; ///< Voltage Log Vendor Specific Size, bytes 25:24
	uint8_t rsvd26[6]; ///< Reserved, bytes 31:26
	uint16_t ovol; ///< Overvoltage, bytes 33:32, see XNVME_SPEC_BASE_LOG_VOLTAGE_MEASUREMENT_OVOL_*
	uint16_t ovc; ///< Overvoltage Count, bytes 35:34
	uint16_t ovlt; ///< Overvoltage Log Threshold, bytes 37:36
	uint64_t ovts; ///< Overvoltage Timestamp, bytes 45:38
	uint8_t iovpe; ///< Interval Overvoltage Percent Error, byte 46
	uint16_t uvol; ///< Undervoltage, bytes 48:47, see XNVME_SPEC_BASE_LOG_VOLTAGE_MEASUREMENT_UVOL_*
	uint16_t uvc; ///< Undervoltage Count, bytes 50:49
	uint16_t uvlt; ///< Undervoltage Log Threshold, bytes 52:51
	uint64_t uvts; ///< Undervoltage Timestamp, bytes 60:53
	uint8_t iuvpe; ///< Interval Undervoltage Percent Error, byte 61
	uint16_t rsvd62; ///< Reserved, bytes 63:62
};

/** Voltage Measurement Counted (Base 2.4, 5.2.13.1.35, Figure 307, page 305, PDF page 333); bit 4 of 'vma' */
#define XNVME_SPEC_BASE_LOG_VOLTAGE_MEASUREMENT_VMA_VMC 0x10
/** Voltage Sensor Measured (Base 2.4, 5.2.13.1.35, Figure 307, page 305, PDF page 333); bits 3:2 of 'vma' */
#define XNVME_SPEC_BASE_LOG_VOLTAGE_MEASUREMENT_VMA_VSM 0xc
/** Interval Voltage Timestamp Support (Base 2.4, 5.2.13.1.35, Figure 307, page 305, PDF page 333); bit 1 of 'vma' */
#define XNVME_SPEC_BASE_LOG_VOLTAGE_MEASUREMENT_VMA_IVOLTS 0x2
/** Voltage Measurement Enable (Base 2.4, 5.2.13.1.35, Figure 307, page 305, PDF page 333); bit 0 of 'vma' */
#define XNVME_SPEC_BASE_LOG_VOLTAGE_MEASUREMENT_VMA_VME 0x1
/** Voltage Sensor Sample Scale (Base 2.4, 5.2.13.1.35, Figure 307, page 305, PDF page 333); bits 15:14 of 'vsi' */
#define XNVME_SPEC_BASE_LOG_VOLTAGE_MEASUREMENT_VSI_VSSS 0xc000
/** Voltage Sensor Supply Label (Base 2.4, 5.2.13.1.35, Figure 307, page 305, PDF page 333); bits 13:12 of 'vsi' */
#define XNVME_SPEC_BASE_LOG_VOLTAGE_MEASUREMENT_VSI_VSSL 0x3000
/** Voltage Sensor Supply Value (Base 2.4, 5.2.13.1.35, Figure 307, page 305, PDF page 333); bits 11:0 of 'vsi' */
#define XNVME_SPEC_BASE_LOG_VOLTAGE_MEASUREMENT_VSI_VSSV 0xfff
/** Overvoltage Valid (Base 2.4, 5.2.13.1.35, Figure 307, page 305, PDF page 333); bit 14 of 'ovol' */
#define XNVME_SPEC_BASE_LOG_VOLTAGE_MEASUREMENT_OVOL_OVV 0x4000
/** Maximum Overvoltage Value (Base 2.4, 5.2.13.1.35, Figure 307, page 305, PDF page 333); bits 13:0 of 'ovol' */
#define XNVME_SPEC_BASE_LOG_VOLTAGE_MEASUREMENT_OVOL_MOVV 0x3fff
/** Undervoltage Valid (Base 2.4, 5.2.13.1.35, Figure 307, page 305, PDF page 333); bit 14 of 'uvol' */
#define XNVME_SPEC_BASE_LOG_VOLTAGE_MEASUREMENT_UVOL_UVV 0x4000
/** Minimum Undervoltage Value (Base 2.4, 5.2.13.1.35, Figure 307, page 305, PDF page 333); bits 13:0 of 'uvol' */
#define XNVME_SPEC_BASE_LOG_VOLTAGE_MEASUREMENT_UVOL_MUVV 0x3fff

/**
 * Sanitize Namespace Status List (Base 2.4, 5.2.13.1.36, Figure 310, page 310, PDF page 338)
 *
 * Followed, from byte 16, by as many NSID as the data says
 */
struct __attribute__((packed)) xnvme_spec_base_log_sanitize_namespace_status_list {
	uint64_t genctr; ///< Generation Counter, bytes 7:0
	uint64_t numnsid; ///< Number of NSIDs, bytes 15:8
};

/**
 * Reservation Notification (Base 2.4, 5.2.13.1.37, Figure 311, page 311, PDF page 339)
 */
struct __attribute__((packed)) xnvme_spec_base_log_reservation_notification {
	uint64_t lpc; ///< Log Page Count, bytes 7:0
	uint8_t rnlpt; ///< Reservation Notification Log Page Type, byte 8
	uint8_t nalp; ///< Number of Available Log Pages, byte 9
	uint16_t rsvd10; ///< Reserved, bytes 11:10
	uint32_t nsid; ///< Namespace ID, bytes 15:12
	uint8_t rsvd16[48]; ///< Reserved, bytes 63:16
};

/**
 * The values of Reservation Notification Log Page Type (Base 2.4, 5.2.13.1.37, Figure 311, page 311, PDF page 339)
 */
enum xnvme_spec_base_log_reservation_notification_rnlpt {
	XNVME_SPEC_BASE_LOG_RESERVATION_NOTIFICATION_RNLPT_EMPTY_LOG_PAGE = 0x0, ///< Empty Log Page (Base 2.4, 5.2.13.1.37, Figure 311, page 311, PDF page 339)
	XNVME_SPEC_BASE_LOG_RESERVATION_NOTIFICATION_RNLPT_REGISTRATION_PREEMPTED = 0x1, ///< Registration Preempted (Base 2.4, 5.2.13.1.37, Figure 311, page 311, PDF page 339)
	XNVME_SPEC_BASE_LOG_RESERVATION_NOTIFICATION_RNLPT_RESERVATION_RELEASED = 0x2, ///< Reservation Released (Base 2.4, 5.2.13.1.37, Figure 311, page 311, PDF page 339)
	XNVME_SPEC_BASE_LOG_RESERVATION_NOTIFICATION_RNLPT_RESERVATION_PREEMPTED = 0x3, ///< Reservation Preempted (Base 2.4, 5.2.13.1.37, Figure 311, page 311, PDF page 339)
};

/**
 * Sanitize Status (Base 2.4, 5.2.13.1.38, Figure 312, page 312, PDF page 340)
 */
struct __attribute__((packed)) xnvme_spec_base_log_sanitize_status {
	uint16_t sprog; ///< Sanitize Progress, bytes 1:0
	uint16_t sstat; ///< Sanitize Status, bytes 3:2, see XNVME_SPEC_BASE_LOG_SANITIZE_STATUS_SSTAT_*
	uint32_t scdw10; ///< Sanitize Command Dword 10 Information, bytes 7:4
	uint32_t eto; ///< Estimated Time For Overwrite, bytes 11:8
	uint32_t etbe; ///< Estimated Time For Block Erase, bytes 15:12
	uint32_t etce; ///< Estimated Time For Crypto Erase, bytes 19:16
	uint32_t etodmm; ///< Estimated Time For Overwrite With No-Deallocate Media Modification, bytes 23:20
	uint32_t etbenmm; ///< Estimated Time For Block Erase With No-Deallocate Media Modification, bytes 27:24
	uint32_t etcenmm; ///< Estimated Time For Crypto Erase With No-Deallocate Media Modification, bytes 31:28
	uint32_t etpvds; ///< Estimated Time For Post-Verification Deallocation State, bytes 35:32
	uint8_t ssi; ///< Sanitize State Information, byte 36, see XNVME_SPEC_BASE_LOG_SANITIZE_STATUS_SSI_*
	uint8_t rsvd37[3]; ///< Reserved, bytes 39:37
	uint32_t mnsoip; ///< Maximum Namespace Sanitize Operations In Progress, bytes 43:40
	uint32_t stnsid; ///< Sanitization Target NSID, bytes 47:44
	uint8_t rsvd48[464]; ///< Reserved, bytes 511:48
};

/** Purged (Base 2.4, 5.2.13.1.38, Figure 312, page 312, PDF page 340); bit 11 of 'sstat' */
#define XNVME_SPEC_BASE_LOG_SANITIZE_STATUS_SSTAT_PRGD 0x800
/** Namespace Data Erased (Base 2.4, 5.2.13.1.38, Figure 312, page 312, PDF page 340); bit 10 of 'sstat' */
#define XNVME_SPEC_BASE_LOG_SANITIZE_STATUS_SSTAT_NDE 0x400
/** Media Verification Canceled (Base 2.4, 5.2.13.1.38, Figure 312, page 312, PDF page 340); bit 9 of 'sstat' */
#define XNVME_SPEC_BASE_LOG_SANITIZE_STATUS_SSTAT_MVCNCLD 0x200
/** Global Data Erased (Base 2.4, 5.2.13.1.38, Figure 312, page 312, PDF page 340); bit 8 of 'sstat' */
#define XNVME_SPEC_BASE_LOG_SANITIZE_STATUS_SSTAT_GDE 0x100
/** Overwrite Passes Completed (Base 2.4, 5.2.13.1.38, Figure 312, page 312, PDF page 340); bits 7:3 of 'sstat' */
#define XNVME_SPEC_BASE_LOG_SANITIZE_STATUS_SSTAT_OPC 0xf8
/** Sanitize Operation Status (Base 2.4, 5.2.13.1.38, Figure 312, page 312, PDF page 340); bits 2:0 of 'sstat' */
#define XNVME_SPEC_BASE_LOG_SANITIZE_STATUS_SSTAT_SOS 0x7
/** Failure State (Base 2.4, 5.2.13.1.38, Figure 312, page 312, PDF page 340); bits 7:4 of 'ssi' */
#define XNVME_SPEC_BASE_LOG_SANITIZE_STATUS_SSI_FAILS 0xf0
/** Sanitize State (Base 2.4, 5.2.13.1.38, Figure 312, page 312, PDF page 340); bits 3:0 of 'ssi' */
#define XNVME_SPEC_BASE_LOG_SANITIZE_STATUS_SSI_SANS 0xf

/**
 * The values of Sanitize State (Base 2.4, 5.2.13.1.38, Figure 312, page 312, PDF page 340)
 */
enum xnvme_spec_base_log_sanitize_status_ssi_sans {
	XNVME_SPEC_BASE_LOG_SANITIZE_STATUS_SSI_SANS_IDLE = 0x0, ///< Idle (Base 2.4, 5.2.13.1.38, Figure 312, page 312, PDF page 340)
	XNVME_SPEC_BASE_LOG_SANITIZE_STATUS_SSI_SANS_RESTRICTED_PROCESSING = 0x1, ///< Restricted Processing (Base 2.4, 5.2.13.1.38, Figure 312, page 312, PDF page 340)
	XNVME_SPEC_BASE_LOG_SANITIZE_STATUS_SSI_SANS_RESTRICTED_FAILURE = 0x2, ///< Restricted Failure (Base 2.4, 5.2.13.1.38, Figure 312, page 312, PDF page 340)
	XNVME_SPEC_BASE_LOG_SANITIZE_STATUS_SSI_SANS_UNRESTRICTED_PROCESSING = 0x3, ///< Unrestricted Processing (Base 2.4, 5.2.13.1.38, Figure 312, page 312, PDF page 340)
	XNVME_SPEC_BASE_LOG_SANITIZE_STATUS_SSI_SANS_UNRESTRICTED_FAILURE = 0x4, ///< Unrestricted Failure (Base 2.4, 5.2.13.1.38, Figure 312, page 312, PDF page 340)
	XNVME_SPEC_BASE_LOG_SANITIZE_STATUS_SSI_SANS_MEDIA_VERIFICATION = 0x5, ///< Media Verification (Base 2.4, 5.2.13.1.38, Figure 312, page 312, PDF page 340)
	XNVME_SPEC_BASE_LOG_SANITIZE_STATUS_SSI_SANS_POST_VERIFICATION_DEALLOCATION = 0x6, ///< Post-Verification Deallocation (Base 2.4, 5.2.13.1.38, Figure 312, page 312, PDF page 340)
};

/**
 * Cross-Controller Reset Entry (Base 2.4, 5.2.13.3.1, Figure 315, page 319, PDF page 347)
 */
struct __attribute__((packed)) xnvme_spec_base_cross_controller_reset_entry {
	uint16_t icid; ///< Impacted Controller ID, bytes 1:0
	uint8_t ciu; ///< Controller Instance Uniquifer, byte 2
	uint8_t rsvd3; ///< Reserved, byte 3
	uint16_t acid; ///< Alternate Controller ID, bytes 5:4
	uint8_t ccrs; ///< Cross-Controller Reset Status, byte 6
	uint8_t ccrf; ///< Cross-Controller Reset Flags, byte 7, see XNVME_SPEC_BASE_CROSS_CONTROLLER_RESET_ENTRY_CCRF_*
};

/** Retry (Base 2.4, 5.2.13.3.1, Figure 315, page 319, PDF page 347); bits 3:2 of 'ccrf' */
#define XNVME_SPEC_BASE_CROSS_CONTROLLER_RESET_ENTRY_CCRF_RETRY 0xc
/** Controller Level Reset Initiated (Base 2.4, 5.2.13.3.1, Figure 315, page 319, PDF page 347); bit 1 of 'ccrf' */
#define XNVME_SPEC_BASE_CROSS_CONTROLLER_RESET_ENTRY_CCRF_CLRI 0x2
/** Validated (Base 2.4, 5.2.13.3.1, Figure 315, page 319, PDF page 347); bit 0 of 'ccrf' */
#define XNVME_SPEC_BASE_CROSS_CONTROLLER_RESET_ENTRY_CCRF_V 0x1

/**
 * The values of Cross-Controller Reset Status (Base 2.4, 5.2.13.3.1, Figure 315, page 319, PDF page 347)
 */
enum xnvme_spec_base_cross_controller_reset_entry_ccrs {
	XNVME_SPEC_BASE_CROSS_CONTROLLER_RESET_ENTRY_CCRS_IN_PROGRESS = 0x0, ///< In Progress (Base 2.4, 5.2.13.3.1, Figure 315, page 319, PDF page 347)
	XNVME_SPEC_BASE_CROSS_CONTROLLER_RESET_ENTRY_CCRS_SUCCESS = 0x1, ///< Success (Base 2.4, 5.2.13.3.1, Figure 315, page 319, PDF page 347)
	XNVME_SPEC_BASE_CROSS_CONTROLLER_RESET_ENTRY_CCRS_FAILED = 0x2, ///< Failed (Base 2.4, 5.2.13.3.1, Figure 315, page 319, PDF page 347)
};

/**
 * Cross-Controller Reset (Base 2.4, 5.2.13.3.1, Figure 314, page 318, PDF page 346)
 */
struct __attribute__((packed)) xnvme_spec_base_log_cross_controller_reset {
	uint16_t ne; ///< Number of Entries, bytes 1:0
	uint8_t rsvd2[6]; ///< Reserved, bytes 7:2
	struct xnvme_spec_base_cross_controller_reset_entry ccre[511]; ///< Cross-Controller Reset Entry, 511 of 8 bytes, bytes 4095:8
};

/**
 * Lost Host Communication Entry (Base 2.4, 5.2.13.3.2, Figure 317, page 322, PDF page 350)
 */
struct __attribute__((packed)) xnvme_spec_base_lost_host_communication_entry {
	uint16_t cntlid; ///< Controller ID, bytes 1:0
	uint8_t lc; ///< Loss Count, byte 2
	uint8_t ciu; ///< Controller Instance Uniquifer, byte 3
	uint32_t rsvd4; ///< Reserved, bytes 7:4
};

/**
 * Lost Host Communication (Base 2.4, 5.2.13.3.2, Figure 316, page 322, PDF page 350)
 */
struct __attribute__((packed)) xnvme_spec_base_log_lost_host_communication {
	uint16_t ne; ///< Number of Entries, bytes 1:0
	uint8_t rsvd2[6]; ///< Reserved, bytes 7:2
	struct xnvme_spec_base_lost_host_communication_entry lhce[511]; ///< Lost Host Communication Entry, 511 of 8 bytes, bytes 4095:8
};

/**
 * Discovery Log Page Entry (Base 2.4, 5.2.13.3.3, Figure 320, page 325, PDF page 353)
 */
struct __attribute__((packed)) xnvme_spec_base_discovery_log_page_entry {
	uint8_t trtype; ///< Transport Type, byte 0
	uint8_t adrfam; ///< Address Family, byte 1
	uint8_t subtype; ///< Subsystem Type, byte 2
	uint8_t treq; ///< Transport Requirements, byte 3, see XNVME_SPEC_BASE_DISCOVERY_LOG_PAGE_ENTRY_TREQ_*
	uint16_t portid; ///< Port ID, bytes 5:4
	uint16_t cntlid; ///< Controller ID, bytes 7:6
	uint16_t asqsz; ///< Admin Max SQ Size, bytes 9:8
	uint16_t eflags; ///< Entry Flags, bytes 11:10, see XNVME_SPEC_BASE_DISCOVERY_LOG_PAGE_ENTRY_EFLAGS_*
	uint8_t rsvd12[20]; ///< Reserved, bytes 31:12
	uint8_t trsvcid[32]; ///< Transport Service Identifier, bytes 63:32
	uint8_t rsvd64[192]; ///< Reserved, bytes 255:64
	uint8_t subnqn[256]; ///< NVM Subsystem Qualified Name, bytes 511:256
	uint8_t traddr[256]; ///< Transport Address, bytes 767:512
	uint8_t tsas[256]; ///< Transport Specific Address Subtype, bytes 1023:768
};

/** Transport Authentication and Secure Channel (Base 2.4, 5.2.13.3.3, Figure 320, page 325, PDF page 353); bits 5:4 of 'treq' */
#define XNVME_SPEC_BASE_DISCOVERY_LOG_PAGE_ENTRY_TREQ_TASC 0x30
/** Zero Host ID Support (Base 2.4, 5.2.13.3.3, Figure 320, page 325, PDF page 353); bit 3 of 'treq' */
#define XNVME_SPEC_BASE_DISCOVERY_LOG_PAGE_ENTRY_TREQ_ZHIDS 0x8
/** SQ Flow Control Disable (Base 2.4, 5.2.13.3.3, Figure 320, page 325, PDF page 353); bit 2 of 'treq' */
#define XNVME_SPEC_BASE_DISCOVERY_LOG_PAGE_ENTRY_TREQ_SQFCD 0x4
/** Transport Secure Channel (Base 2.4, 5.2.13.3.3, Figure 320, page 325, PDF page 353); bits 1:0 of 'treq' */
#define XNVME_SPEC_BASE_DISCOVERY_LOG_PAGE_ENTRY_TREQ_TSC 0x3
/** No CDC Connectivity (Base 2.4, 5.2.13.3.3, Figure 320, page 325, PDF page 353); bit 2 of 'eflags' */
#define XNVME_SPEC_BASE_DISCOVERY_LOG_PAGE_ENTRY_EFLAGS_NCC 0x4
/** Explicit Persistent Connection Support for Discovery (Base 2.4, 5.2.13.3.3, Figure 320, page 325, PDF page 353); bit 1 of 'eflags' */
#define XNVME_SPEC_BASE_DISCOVERY_LOG_PAGE_ENTRY_EFLAGS_EPCSD 0x2
/** Duplicate Returned Information (Base 2.4, 5.2.13.3.3, Figure 320, page 325, PDF page 353); bit 0 of 'eflags' */
#define XNVME_SPEC_BASE_DISCOVERY_LOG_PAGE_ENTRY_EFLAGS_DUPRETINFO 0x1

/**
 * The values of Transport Type (Base 2.4, 5.2.13.3.3, Figure 320, page 325, PDF page 353)
 */
enum xnvme_spec_base_discovery_log_page_entry_trtype {
	XNVME_SPEC_BASE_DISCOVERY_LOG_PAGE_ENTRY_TRTYPE_RDMA_TRANSPORT = 0x1, ///< RDMA Transport (Base 2.4, 5.2.13.3.3, Figure 320, page 325, PDF page 353)
	XNVME_SPEC_BASE_DISCOVERY_LOG_PAGE_ENTRY_TRTYPE_FIBRE_CHANNEL_TRANSPORT = 0x2, ///< Fibre Channel Transport (Base 2.4, 5.2.13.3.3, Figure 320, page 325, PDF page 353)
	XNVME_SPEC_BASE_DISCOVERY_LOG_PAGE_ENTRY_TRTYPE_TCP_TRANSPORT = 0x3, ///< TCP Transport (Base 2.4, 5.2.13.3.3, Figure 320, page 325, PDF page 353)
};

/**
 * The values of Address Family (Base 2.4, 5.2.13.3.3, Figure 320, page 325, PDF page 353)
 */
enum xnvme_spec_base_discovery_log_page_entry_adrfam {
	XNVME_SPEC_BASE_DISCOVERY_LOG_PAGE_ENTRY_ADRFAM_AF_INET = 0x1, ///< AF_INET (Base 2.4, 5.2.13.3.3, Figure 320, page 325, PDF page 353)
	XNVME_SPEC_BASE_DISCOVERY_LOG_PAGE_ENTRY_ADRFAM_AF_INET6 = 0x2, ///< AF_INET6 (Base 2.4, 5.2.13.3.3, Figure 320, page 325, PDF page 353)
	XNVME_SPEC_BASE_DISCOVERY_LOG_PAGE_ENTRY_ADRFAM_AF_IB = 0x3, ///< AF_IB (Base 2.4, 5.2.13.3.3, Figure 320, page 325, PDF page 353)
};

/**
 * The values of Subsystem Type (Base 2.4, 5.2.13.3.3, Figure 320, page 325, PDF page 353)
 */
enum xnvme_spec_base_discovery_log_page_entry_subtype {
	XNVME_SPEC_BASE_DISCOVERY_LOG_PAGE_ENTRY_SUBTYPE_REFERRAL = 0x1, ///< Referral (Base 2.4, 5.2.13.3.3, Figure 320, page 325, PDF page 353)
	XNVME_SPEC_BASE_DISCOVERY_LOG_PAGE_ENTRY_SUBTYPE_NVM_SUBSYSTEM = 0x2, ///< NVM Subsystem (Base 2.4, 5.2.13.3.3, Figure 320, page 325, PDF page 353)
	XNVME_SPEC_BASE_DISCOVERY_LOG_PAGE_ENTRY_SUBTYPE_CURRENT_DISCOVERY_SUBSYSTEM = 0x3, ///< Current Discovery Subsystem (Base 2.4, 5.2.13.3.3, Figure 320, page 325, PDF page 353)
};

/**
 * The values of Transport Secure Channel (Base 2.4, 5.2.13.3.3, Figure 320, page 325, PDF page 353)
 */
enum xnvme_spec_base_discovery_log_page_entry_treq_tsc {
	XNVME_SPEC_BASE_DISCOVERY_LOG_PAGE_ENTRY_TREQ_TSC_REQUIRED = 0x1, ///< Required (Base 2.4, 5.2.13.3.3, Figure 320, page 325, PDF page 353)
	XNVME_SPEC_BASE_DISCOVERY_LOG_PAGE_ENTRY_TREQ_TSC_NOT_REQUIRED = 0x2, ///< Not required (Base 2.4, 5.2.13.3.3, Figure 320, page 325, PDF page 353)
};

/**
 * Extended Discovery (Base 2.4, 5.2.13.3.3, Figure 321, page 328, PDF page 356)
 *
 * Followed, from byte 1032, by as many Extended Attribute as the data says
 *
 * Not members:
 * Extended Attribute N: at an offset the data gives, in 'extended_attribute'
 */
struct __attribute__((packed)) xnvme_spec_base_log_extended_discovery {
	uint8_t trtype; ///< Transport Type, byte 0
	uint8_t adrfam; ///< Address Family, byte 1
	uint8_t subtype; ///< Subsystem Type, byte 2
	uint8_t treq; ///< Transport Requirements, byte 3
	uint16_t portid; ///< Port ID, bytes 5:4
	uint16_t cntlid; ///< Controller ID, bytes 7:6
	uint16_t asqsz; ///< Admin Max SQ Size, bytes 9:8
	uint16_t eflags; ///< Entry Flags, bytes 11:10
	uint8_t rsvd12[20]; ///< Reserved, bytes 31:12
	uint8_t trsvcid[32]; ///< Transport Service Identifier, bytes 63:32
	uint8_t rsvd64[192]; ///< Reserved, bytes 255:64
	uint8_t nqn[256]; ///< NVM Subsystem Qualified Name, bytes 511:256
	uint8_t traddr[256]; ///< Transport Address, bytes 767:512
	uint8_t tsas[256]; ///< Transport Specific Address Subtype, bytes 1023:768
	uint32_t tel; ///< Total Entry Length, bytes 1027:1024
	uint16_t numexat; ///< Number of Extended Attributes, bytes 1029:1028
	uint16_t rsvd1030; ///< Reserved, bytes 1031:1030
};

/**
 * Discovery (Base 2.4, 5.2.13.3.3, Figure 322, page 329, PDF page 357)
 *
 * Followed, from byte 1024, by as many Entry as the data says
 *
 * Not members:
 * Entry N: at an offset the data gives, in 'entry'
 */
struct __attribute__((packed)) xnvme_spec_base_log_discovery {
	uint64_t genctr; ///< Generation Counter, bytes 7:0
	uint64_t numrec; ///< Number of Records, bytes 15:8
	uint16_t recfmt; ///< Record Format, bytes 17:16
	uint8_t dlpf; ///< Discovery Log Page Flags, byte 18, see XNVME_SPEC_BASE_LOG_DISCOVERY_DLPF_*
	uint8_t rsvd19; ///< Reserved, byte 19
	uint32_t tdlpl; ///< Total Discovery Log Page Length, bytes 23:20
	uint8_t rsvd24[1000]; ///< Reserved, bytes 1023:24
};

/** All Subsystems (Base 2.4, 5.2.13.3.3, Figure 322, page 329, PDF page 357); bit 2 of 'dlpf' */
#define XNVME_SPEC_BASE_LOG_DISCOVERY_DLPF_ALLSUB 0x4
/** Port Local (Base 2.4, 5.2.13.3.3, Figure 322, page 329, PDF page 357); bit 1 of 'dlpf' */
#define XNVME_SPEC_BASE_LOG_DISCOVERY_DLPF_PORTLCL 0x2
/** Extended (Base 2.4, 5.2.13.3.3, Figure 322, page 329, PDF page 357); bit 0 of 'dlpf' */
#define XNVME_SPEC_BASE_LOG_DISCOVERY_DLPF_EXTEND 0x1

/**
 * Host Extended Discovery (Base 2.4, 5.2.13.3.4, Figure 325, page 331, PDF page 359)
 *
 * Followed, from byte 1032, by as many Extended Attribute as the data says
 *
 * Not members:
 * Extended Attribute N: at an offset the data gives, in 'extended_attribute'
 */
struct __attribute__((packed)) xnvme_spec_base_log_host_extended_discovery {
	uint8_t trtype; ///< Transport Type, byte 0
	uint8_t adrfam; ///< Address Family, byte 1
	uint64_t rsvd2; ///< Reserved, bytes 9:2
	uint16_t eflags; ///< Entry Flags, bytes 11:10, see XNVME_SPEC_BASE_LOG_HOST_EXTENDED_DISCOVERY_EFLAGS_*
	uint8_t rsvd12[244]; ///< Reserved, bytes 255:12
	uint8_t hostnqn[256]; ///< Host NVMe Qualified Name, bytes 511:256
	uint8_t traddr[256]; ///< Transport Address, bytes 767:512
	uint8_t tsas[256]; ///< Transport Specific Address Subtype, bytes 1023:768
	uint32_t tel; ///< Total Entry Length, bytes 1027:1024
	uint16_t numexat; ///< Number of Extended Attributes, bytes 1029:1028
	uint16_t rsvd1030; ///< Reserved, bytes 1031:1030
};

/** No CDC Connectivity (Base 2.4, 5.2.13.3.4, Figure 325, page 331, PDF page 359); bit 2 of 'eflags' */
#define XNVME_SPEC_BASE_LOG_HOST_EXTENDED_DISCOVERY_EFLAGS_NCC 0x4

/**
 * Host Discovery (Base 2.4, 5.2.13.3.4, Figure 326, page 332, PDF page 360)
 *
 * Followed, from byte 1024, by as many Host Extended Discovery Log Page Entry as the data says
 *
 * Not members:
 * Host Extended Discovery Log Page Entry N: at an offset the data gives, in 'host_extended_discovery_log_page_entry'
 */
struct __attribute__((packed)) xnvme_spec_base_log_host_discovery {
	uint64_t genctr; ///< Generation Counter, bytes 7:0
	uint64_t numrec; ///< Number of Records, bytes 15:8
	uint16_t recfmt; ///< Record Format, bytes 17:16
	uint8_t hdlpf; ///< Host Discovery Log Page Flags, byte 18, see XNVME_SPEC_BASE_LOG_HOST_DISCOVERY_HDLPF_*
	uint8_t rsvd19; ///< Reserved, byte 19
	uint32_t thdlpl; ///< Total Host Discovery Log Page Length, bytes 23:20
	uint8_t rsvd24[1000]; ///< Reserved, bytes 1023:24
};

/** All Hosts (Base 2.4, 5.2.13.3.4, Figure 326, page 332, PDF page 360); bit 0 of 'hdlpf' */
#define XNVME_SPEC_BASE_LOG_HOST_DISCOVERY_HDLPF_ALLHOST 0x1

/**
 * Get Log Page - AVE Discovery (Base 2.4, 5.2.13.3.5, Figure 327, page 332, PDF page 360)
 *
 * Followed, from byte 1024, by as many AVE Discovery Log Page Entry as the data says
 *
 * Not members:
 * AVE Discovery Log Page Entry NUMREC-1: at an offset the data gives, in 'ave_discovery_log_page_entry'
 */
struct __attribute__((packed)) xnvme_spec_base_log_ave_discovery {
	uint64_t genctr; ///< Generation Counter, bytes 7:0
	uint64_t numrec; ///< Number of Records, bytes 15:8
	uint16_t recfmt; ///< Record Format, bytes 17:16
	uint16_t rsvd18; ///< Reserved, bytes 19:18
	uint32_t tadlpl; ///< Total AVE Discovery Log Page Length, bytes 23:20
	uint8_t rsvd24[1000]; ///< Reserved, bytes 1023:24
};

/**
 * AVE Transport Record (Base 2.4, 5.2.13.3.5, Figure 329, page 333, PDF page 361)
 */
struct __attribute__((packed)) xnvme_spec_base_ave_transport_record {
	uint8_t aveadrfam; ///< AVE Address Family, byte 0
	uint8_t rsvd1; ///< Reserved, byte 1
	uint16_t avetrsvcid; ///< AVE Transport Service Identifier, bytes 3:2
	uint8_t avetraddr[16]; ///< AVE Transport Address, bytes 19:4
};

/**
 * Get Log Page - AVE Discovery (Base 2.4, 5.2.13.3.5, Figure 328, page 333, PDF page 361)
 *
 * Followed, from byte 232, by as many AVE Transport Record as the data says, each a 'struct xnvme_spec_base_ave_transport_record'
 */
struct __attribute__((packed)) xnvme_spec_base_log_ave_discovery_figure328 {
	uint32_t tel; ///< Total Entry Length, bytes 3:0
	uint8_t avenqn[224]; ///< AVE NQN, bytes 227:4
	uint8_t numatr; ///< Number of AVE Transport Records, byte 228
	uint8_t rsvd229[3]; ///< Reserved, bytes 231:229
};

/**
 * Pull Model DDC Request (Base 2.4, 5.2.13.3.6, Figure 330, page 334, PDF page 362)
 *
 * Followed, from byte 8, by as many Operation Specific Parameters as the data says
 */
struct __attribute__((packed)) xnvme_spec_base_log_pull_model_ddc_request {
	uint8_t ori; ///< Operation Request Identifier, byte 0
	uint8_t rsvd1[3]; ///< Reserved, bytes 3:1
	uint32_t tpdrpl; ///< Total Pull Model DDC Request Log Page Length, bytes 7:4
};

/**
 * Identify - Power State Descriptor (Base 2.4, 5.2.14.2.1, Figure 340, page 382, PDF page 410), one entry of 32 bytes
 */
struct __attribute__((packed)) xnvme_spec_base_power_state_descriptor {
	uint16_t mp; ///< Maximum Power, bytes 1:0
	uint8_t rsvd2; ///< Reserved
	uint8_t mxps_nops; ///< Max Power Scale; Non-Operational State, byte 3, see XNVME_SPEC_BASE_POWER_STATE_DESCRIPTOR_MXPS_NOPS_*
	uint32_t enlat; ///< Entry Latency, bytes 7:4
	uint32_t exlat; ///< Exit Latency, bytes 11:8
	uint8_t rrt; ///< Relative Read Throughput, bits 4:0 of byte 12
	uint8_t rrl; ///< Relative Read Latency, bits 4:0 of byte 13
	uint8_t rwt; ///< Relative Write Throughput, bits 4:0 of byte 14
	uint8_t rwl; ///< Relative Write Latency, bits 4:0 of byte 15
	uint16_t idlp; ///< Idle Power, bytes 17:16
	uint8_t ips; ///< Idle Power Scale, byte 18, see XNVME_SPEC_BASE_POWER_STATE_DESCRIPTOR_IPS_*
	uint8_t rsvd19; ///< Reserved
	uint16_t actp; ///< Active Power, bytes 21:20
	uint8_t apw_aps; ///< Active Power Workload; Active Power Scale, byte 22, see XNVME_SPEC_BASE_POWER_STATE_DESCRIPTOR_APW_APS_*
	uint8_t epfrt; ///< Emergency Power Fail Recovery Time, byte 23
	uint8_t fqvt; ///< Forced Quiescence Vault Time, byte 24
	uint8_t epfvt; ///< Emergency Power Fail Vault Time, byte 25
	uint8_t epfrts_fqvts; ///< Emergency Power Fail Recovery Time Scale; Forced Quiescence Vault Time Scale, byte 26, see XNVME_SPEC_BASE_POWER_STATE_DESCRIPTOR_EPFRTS_FQVTS_*
	uint8_t epfvts; ///< Emergency Power Fail Vault Time Scale, bits 3:0 of byte 27
	uint8_t mbw; ///< Max Bandwidth, byte 28
	uint8_t mbws; ///< Max Bandwidth Scale, bits 2:0 of byte 29
	uint16_t miiell; ///< Minimum Idle I/O Exit Latency Limit, bytes 31:30
};

/** Max Power Scale (Base 2.4, 5.2.14.2.1, Figure 340, page 382, PDF page 410); bit 0 of 'mxps_nops' */
#define XNVME_SPEC_BASE_POWER_STATE_DESCRIPTOR_MXPS_NOPS_MXPS 0x1
/** Non-Operational State (Base 2.4, 5.2.14.2.1, Figure 340, page 382, PDF page 410); bit 1 of 'mxps_nops' */
#define XNVME_SPEC_BASE_POWER_STATE_DESCRIPTOR_MXPS_NOPS_NOPS 0x2
/** Idle Power Scale (Base 2.4, 5.2.14.2.1, Figure 340, page 382, PDF page 410); bits 7:6 of 'ips' */
#define XNVME_SPEC_BASE_POWER_STATE_DESCRIPTOR_IPS_IPS 0xc0
/** Active Power Workload (Base 2.4, 5.2.14.2.1, Figure 340, page 382, PDF page 410); bits 2:0 of 'apw_aps' */
#define XNVME_SPEC_BASE_POWER_STATE_DESCRIPTOR_APW_APS_APW 0x7
/** Active Power Scale (Base 2.4, 5.2.14.2.1, Figure 340, page 382, PDF page 410); bits 7:6 of 'apw_aps' */
#define XNVME_SPEC_BASE_POWER_STATE_DESCRIPTOR_APW_APS_APS 0xc0
/** Emergency Power Fail Recovery Time Scale (Base 2.4, 5.2.14.2.1, Figure 340, page 382, PDF page 410); bits 3:0 of 'epfrts_fqvts' */
#define XNVME_SPEC_BASE_POWER_STATE_DESCRIPTOR_EPFRTS_FQVTS_EPFRTS 0xf
/** Forced Quiescence Vault Time Scale (Base 2.4, 5.2.14.2.1, Figure 340, page 382, PDF page 410); bits 7:4 of 'epfrts_fqvts' */
#define XNVME_SPEC_BASE_POWER_STATE_DESCRIPTOR_EPFRTS_FQVTS_FQVTS 0xf0

/**
 * The values of Idle Power Scale (Base 2.4, 5.2.14.2.1, Figure 340, page 382, PDF page 410)
 */
enum xnvme_spec_base_power_state_descriptor_ips_ips {
	XNVME_SPEC_BASE_POWER_STATE_DESCRIPTOR_IPS_IPS_0_0001_W = 0x1, ///< 0.0001 W (Base 2.4, 5.2.14.2.1, Figure 340, page 382, PDF page 410)
	XNVME_SPEC_BASE_POWER_STATE_DESCRIPTOR_IPS_IPS_0_01_W = 0x2, ///< 0.01 W (Base 2.4, 5.2.14.2.1, Figure 340, page 382, PDF page 410)
};

/**
 * The values of Active Power Scale (Base 2.4, 5.2.14.2.1, Figure 340, page 382, PDF page 410)
 */
enum xnvme_spec_base_power_state_descriptor_apw_aps_aps {
	XNVME_SPEC_BASE_POWER_STATE_DESCRIPTOR_APW_APS_APS_0_0001_W = 0x1, ///< 0.0001 W (Base 2.4, 5.2.14.2.1, Figure 340, page 382, PDF page 410)
	XNVME_SPEC_BASE_POWER_STATE_DESCRIPTOR_APW_APS_APS_0_01_W = 0x2, ///< 0.01 W (Base 2.4, 5.2.14.2.1, Figure 340, page 382, PDF page 410)
};

/**
 * The values of Max Bandwidth Scale (Base 2.4, 5.2.14.2.1, Figure 340, page 382, PDF page 410)
 */
enum xnvme_spec_base_power_state_descriptor_mbws {
	XNVME_SPEC_BASE_POWER_STATE_DESCRIPTOR_MBWS_1_MIB_SECOND = 0x0, ///< 1 MiB/second (Base 2.4, 5.2.14.2.1, Figure 340, page 382, PDF page 410)
	XNVME_SPEC_BASE_POWER_STATE_DESCRIPTOR_MBWS_10_MIB_SECOND = 0x1, ///< 10 MiB/second (Base 2.4, 5.2.14.2.1, Figure 340, page 382, PDF page 410)
	XNVME_SPEC_BASE_POWER_STATE_DESCRIPTOR_MBWS_100_MIB_SECOND = 0x2, ///< 100 MiB/second (Base 2.4, 5.2.14.2.1, Figure 340, page 382, PDF page 410)
	XNVME_SPEC_BASE_POWER_STATE_DESCRIPTOR_MBWS_1_GIB_SECOND = 0x3, ///< 1 GiB/second (Base 2.4, 5.2.14.2.1, Figure 340, page 382, PDF page 410)
	XNVME_SPEC_BASE_POWER_STATE_DESCRIPTOR_MBWS_10_GIB_SECOND = 0x4, ///< 10 GiB/second (Base 2.4, 5.2.14.2.1, Figure 340, page 382, PDF page 410)
	XNVME_SPEC_BASE_POWER_STATE_DESCRIPTOR_MBWS_100_GIB_SECOND = 0x5, ///< 100 GiB/second (Base 2.4, 5.2.14.2.1, Figure 340, page 382, PDF page 410)
};

/**
 * Identify - Identify Controller (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366)
 */
struct __attribute__((packed)) xnvme_spec_base_identify_controller {
	uint16_t vid; ///< PCI Vendor ID, bytes 1:0
	uint16_t ssvid; ///< PCI Subsystem Vendor ID, bytes 3:2
	uint8_t sn[20]; ///< Serial Number, bytes 23:4
	uint8_t mn[40]; ///< Model Number, bytes 63:24
	uint8_t fr[8]; ///< Firmware Revision, bytes 71:64
	uint8_t rab; ///< Recommended Arbitration Burst, byte 72
	uint8_t ieee[3]; ///< IEEE OUI Identifier, bytes 75:73
	uint8_t cmic; ///< Controller Multi-Path I/O and Namespace Sharing Capabilities, byte 76, see XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_CMIC_*
	uint8_t mdts; ///< Maximum Data Transfer Size, byte 77
	uint16_t cntlid; ///< Controller ID, bytes 79:78
	uint32_t ver; ///< Version, bytes 83:80
	uint32_t rtd3r; ///< RTD3 Resume Latency, bytes 87:84
	uint32_t rtd3e; ///< RTD3 Entry Latency, bytes 91:88
	uint32_t oaes; ///< Optional Asynchronous Events Supported, bytes 95:92, see XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_OAES_*
	uint32_t ctratt; ///< Controller Attributes, bytes 99:96, see XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_CTRATT_*
	uint16_t rrls; ///< Read Recovery Levels Supported, bytes 101:100, see XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_RRLS_*
	uint8_t bpcap; ///< Boot Partition Capabilities, byte 102, see XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_BPCAP_*
	uint8_t chsi; ///< CXL HDM Support Information, byte 103, see XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_CHSI_*
	uint32_t nssl; ///< NVM Subsystem Shutdown Latency, bytes 107:104
	uint16_t rsvd108; ///< Reserved, bytes 109:108
	uint8_t plsi; ///< Power Loss Signaling Information, byte 110, see XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_PLSI_*
	uint8_t cntrltype; ///< Controller Type, byte 111
	uint8_t fguid[16]; ///< FRU Globally Unique Identifier, bytes 127:112
	uint16_t crdt[3]; ///< Command Retry Delay Time, 3 of 2 bytes, bytes 133:128
	uint8_t crcap; ///< Controller Reachability Capabilities, byte 134, see XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_CRCAP_*
	uint8_t ciu; ///< Controller Instance Uniquifier, byte 135
	uint64_t cirn; ///< Controller Instance Random Number, bytes 143:136
	uint8_t rsvd144[96]; ///< Reserved, bytes 239:144
	uint8_t rsvd240[13]; ///< Reserved, bytes 252:240
	uint8_t nvmsr; ///< NVM Subsystem Report, byte 253, see XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_NVMSR_*
	uint8_t vwci; ///< VPD Write Cycle Information, byte 254, see XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_VWCI_*
	uint8_t mec; ///< Management Endpoint Capabilities, byte 255, see XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_MEC_*
	uint16_t oacs; ///< Optional Admin Command Support, bytes 257:256, see XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_OACS_*
	uint8_t acl; ///< Abort Command Limit, byte 258
	uint8_t aerl; ///< Asynchronous Event Request Limit, byte 259
	uint8_t frmw; ///< Firmware Updates, byte 260, see XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_FRMW_*
	uint8_t lpa; ///< Log Page Attributes, byte 261, see XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_LPA_*
	uint8_t elpe; ///< Error Log Page Entries, byte 262
	uint8_t npss; ///< Number of Power States Support, byte 263
	uint8_t avscc; ///< Admin Vendor Specific Command Configuration, byte 264, see XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_AVSCC_*
	uint8_t apsta; ///< Autonomous Power State Transition Attributes, byte 265, see XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_APSTA_*
	uint16_t wctemp; ///< Warning Composite Temperature Threshold, bytes 267:266
	uint16_t cctemp; ///< Critical Composite Temperature Threshold, bytes 269:268
	uint16_t mtfa; ///< Maximum Time for Firmware Activation, bytes 271:270
	uint32_t hmpre; ///< Host Memory Buffer Preferred Size, bytes 275:272
	uint32_t hmmin; ///< Host Memory Buffer Minimum Size, bytes 279:276
	uint8_t tnvmcap[16]; ///< Total NVM Capacity, bytes 295:280
	uint8_t unvmcap[16]; ///< Unallocated NVM Capacity, bytes 311:296
	uint32_t rpmbs; ///< Replay Protected Memory Block Support, bytes 315:312, see XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_RPMBS_*
	uint16_t edstt; ///< Extended Device Self-test Time, bytes 317:316
	uint8_t dsto; ///< Device Self-test Options, byte 318, see XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_DSTO_*
	uint8_t fwug; ///< Firmware Update Granularity, byte 319
	uint16_t kas; ///< Keep Alive Support, bytes 321:320
	uint16_t hctma; ///< Host Controlled Thermal Management Attributes, bytes 323:322, see XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_HCTMA_*
	uint16_t mntmt; ///< Minimum Thermal Management Temperature, bytes 325:324
	uint16_t mxtmt; ///< Maximum Thermal Management Temperature, bytes 327:326
	uint32_t sanicap; ///< Sanitize Capabilities, bytes 331:328, see XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_SANICAP_*
	uint32_t hmminds; ///< Host Memory Buffer Minimum Descriptor Entry Size, bytes 335:332
	uint16_t hmmaxd; ///< Host Memory Maximum Descriptors Entries, bytes 337:336
	uint16_t nsetidmax; ///< NVM Set Identifier Maximum, bytes 339:338
	uint16_t endgidmax; ///< Endurance Group Identifier Maximum, bytes 341:340
	uint8_t anatt; ///< ANA Transition Time, byte 342
	uint8_t anacap; ///< Asymmetric Namespace Access Capabilities, byte 343, see XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_ANACAP_*
	uint32_t anagrpmax; ///< ANA Group Identifier Maximum, bytes 347:344
	uint32_t nanagrpid; ///< Number of ANA Group Identifiers, bytes 351:348
	uint32_t pels; ///< Persistent Event Log Size, bytes 355:352
	uint16_t did; ///< Domain Identifier, bytes 357:356
	uint8_t kpioc; ///< Key Per I/O Capabilities, byte 358, see XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_KPIOC_*
	uint8_t rsvd359; ///< Reserved, byte 359
	uint16_t mptfawr; ///< Maximum Processing Time for Firmware Activation Without Reset, bytes 361:360
	uint8_t rmdca; ///< Restore Manufacturing Configuration Attributes, byte 362, see XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_RMDCA_*
	uint8_t rsvd363[5]; ///< Reserved, bytes 367:363
	uint8_t megcap[16]; ///< Max Endurance Group Capacity, bytes 383:368
	uint8_t tmpthha; ///< Temperature Threshold Hysteresis Attributes, byte 384, see XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_TMPTHHA_*
	uint8_t mupa; ///< Maximum Unlimited Power Attributes, byte 385, see XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_MUPA_*
	uint16_t cqt; ///< Command Quiesce Time, bytes 387:386
	uint16_t cdpa; ///< Configurable Device Personality Attributes, bytes 389:388, see XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_CDPA_*
	uint16_t mup; ///< Maximum Unlimited Power, bytes 391:390
	uint16_t ipmsr; ///< Interval Power Measurement Sample Rate, bytes 393:392, see XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_IPMSR_*
	uint16_t msmt; ///< Maximum Stop Measurement Time, bytes 395:394
	uint16_t mnens; ///< Maximum Number of Exported NVM Subsystems, bytes 397:396
	uint16_t mnecpens; ///< Maximum Number of Exported Controllers per Exported NVM Subsystem, bytes 399:398
	uint32_t mensnn; ///< Maximum Exported NVM Subsystem Number of Namespaces, bytes 403:400
	uint8_t ensa; ///< Exported NVM Subsystem Attributes, byte 404, see XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_ENSA_*
	uint8_t endsfs; ///< Exported Namespace Data Structure Formats Supported, byte 405, see XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_ENDSFS_*
	uint32_t vsen[4]; ///< Voltage Sensor, 4 of 4 bytes of XNVME_SPEC_BASE_VOLTAGE_SENSOR_*, bytes 421:406
	uint16_t msvmt; ///< Maximum Stop Voltage Measurement Time, bytes 423:422
	uint8_t rsvd424[88]; ///< Reserved, bytes 511:424
	uint8_t sqes; ///< Submission Queue Entry Size, byte 512, see XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_SQES_*
	uint8_t cqes; ///< Completion Queue Entry Size, byte 513, see XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_CQES_*
	uint16_t maxcmd; ///< Maximum Outstanding Commands, bytes 515:514
	uint32_t nn; ///< Number of Namespaces, bytes 519:516
	uint16_t oncs; ///< Optional NVM Command Support, bytes 521:520, see XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_ONCS_*
	uint16_t fuses; ///< Fused Operation Support, bytes 523:522, see XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_FUSES_*
	uint8_t fna; ///< Format NVM Attributes, byte 524, see XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_FNA_*
	uint8_t vwc; ///< Volatile Write Cache, byte 525, see XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_VWC_*
	uint16_t awun; ///< Atomic Write Unit Normal, bytes 527:526
	uint16_t awupf; ///< Atomic Write Unit Power Fail, bytes 529:528
	uint8_t icsvscc; ///< I/O Command Set Vendor Specific Command Configuration, byte 530, see XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_ICSVSCC_*
	uint8_t nwpc; ///< Namespace Write Protection Capabilities, byte 531, see XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_NWPC_*
	uint16_t acwu; ///< Atomic Compare & Write Unit, bytes 533:532
	uint16_t cdfs; ///< Copy Descriptor Formats Supported, bytes 535:534, see XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_CDFS_*
	uint32_t sgls; ///< SGL Support, bytes 539:536, see XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_SGLS_*
	uint32_t mnan; ///< Maximum Number of Allowed Namespaces, bytes 543:540
	uint8_t maxdna[16]; ///< Maximum Domain Namespace Attachments, bytes 559:544
	uint32_t maxcna; ///< Maximum I/O Controller Namespace Attachments, bytes 563:560
	uint32_t oaqd; ///< Optimal Aggregated Queue Depth, bytes 567:564
	uint8_t rhiri; ///< Recommended Host-Initiated Refresh Interval, byte 568
	uint8_t hirt; ///< Host-Initiated Refresh Time, byte 569
	uint16_t cmmrtd; ///< Controller Maximum Memory Range Tracking Descriptors, bytes 571:570
	uint16_t nmmrtd; ///< NVM Subsystem Maximum Memory Range Tracking Descriptors, bytes 573:572
	uint8_t minmrtg; ///< Minimum Memory Range Tracking Granularity, byte 574
	uint8_t maxmrtg; ///< Maximum Memory Range Tracking Granularity, byte 575
	uint8_t trattr; ///< Tracking Attributes, byte 576, see XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_TRATTR_*
	uint8_t rsvd577; ///< Reserved, byte 577
	uint16_t mcudmq; ///< Maximum Controller User Data Migration Queues, bytes 579:578
	uint16_t mnsudmq; ///< Maximum NVM Subsystem User Data Migration Queues, bytes 581:580
	uint16_t mcmr; ///< Maximum CDQ Memory Ranges, bytes 583:582
	uint16_t nmcmr; ///< NVM Subsystem Maximum CDQ Memory Ranges, bytes 585:584
	uint16_t mcdqpc; ///< Maximum Controller Data Queue PRP Count, bytes 587:586
	uint8_t rsvd588[180]; ///< Reserved, bytes 767:588
	uint8_t subnqn[256]; ///< NVM Subsystem NVMe Qualified Name, bytes 1023:768
	uint8_t rsvd1024[768]; ///< Reserved, bytes 1791:1024
	uint32_t ioccsz; ///< I/O Queue Command Capsule Supported Size, bytes 1795:1792
	uint32_t iorcsz; ///< I/O Queue Response Capsule Supported Size, bytes 1799:1796
	uint16_t icdoff; ///< In Capsule Data Offset, bytes 1801:1800
	uint8_t fcatt; ///< Fabrics Controller Attributes, byte 1802, see XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_FCATT_*
	uint8_t msdbd; ///< Maximum SGL Data Block Descriptors, byte 1803
	uint16_t ofcs; ///< Optional Fabrics Commands Support, bytes 1805:1804, see XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_OFCS_*
	uint8_t dctype; ///< Discovery Controller Type, byte 1806
	uint8_t ccrl; ///< Cross-Controller Reset Limit, byte 1807
	uint8_t rsvd1808[240]; ///< Reserved, bytes 2047:1808
	struct xnvme_spec_base_power_state_descriptor psd[32]; ///< Power State Descriptor, 32 of 32 bytes, bytes 3071:2048
	uint8_t vs[1024]; ///< Vendor Specific, bytes 4095:3072
};

/** Asymmetric Namespace Access Reporting Support (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 3 of 'cmic' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_CMIC_ANARS 0x8
/** Function Type (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 2 of 'cmic' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_CMIC_FT 0x4
/** Multiple Controllers (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 1 of 'cmic' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_CMIC_MCTRS 0x2
/** Multiple Ports (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 0 of 'cmic' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_CMIC_MPORTS 0x1
/** Discovery Log Page Change Notification (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 31 of 'oaes' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_OAES_DLPCN 0x80000000
/** Zone Descriptor Changed Notices (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 27 of 'oaes' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_OAES_ZDCN 0x8000000
/** Rate Limiting Configuration Change (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 22 of 'oaes' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_OAES_RLCC 0x400000
/** Lost Host Communication Notices (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 21 of 'oaes' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_OAES_LHCN 0x200000
/** Cross-Controller Reset Completed Notices (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 20 of 'oaes' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_OAES_CCRCN 0x100000
/** Allocated Namespace Attribute Notices (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 19 of 'oaes' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_OAES_ANSAN 0x80000
/** Reachability Groups Change Notices Support (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 17 of 'oaes' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_OAES_RGCNS 0x20000
/** Temperature Threshold Hysteresis Recovery (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 16 of 'oaes' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_OAES_TTHR 0x10000
/** Normal NVM Subsystem Shutdown (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 15 of 'oaes' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_OAES_NNSS 0x8000
/** Endurance Group Event Aggregate Log Page Change Notices (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 14 of 'oaes' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_OAES_EGEAN 0x4000
/** LBA Status Information Alert Notices (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 13 of 'oaes' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_OAES_LSIAN 0x2000
/** Predictable Latency Event Aggregate Log Change Notices (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 12 of 'oaes' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_OAES_PLEAN 0x1000
/** Asymmetric Namespace Access Change Notices (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 11 of 'oaes' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_OAES_ANACN 0x800
/** Firmware Activation Notices (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 9 of 'oaes' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_OAES_FAN 0x200
/** Attached Namespace Attribute Notices (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 8 of 'oaes' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_OAES_NSAN 0x100
/** Idle I/O Exit Latency Limit Support and Scope (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bits 24:23 of 'ctratt' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_CTRATT_IIELLSS 0x1800000
/** Voltage Measurement Support (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 22 of 'ctratt' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_CTRATT_VMS 0x400000
/** Power Measurement Support (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 21 of 'ctratt' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_CTRATT_PMS 0x200000
/** Power Limit Support (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 20 of 'ctratt' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_CTRATT_PLS 0x100000
/** Flexible Data Placement Support (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 19 of 'ctratt' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_CTRATT_FDPS 0x80000
/** Reservations and Host Identifier Interaction (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 18 of 'ctratt' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_CTRATT_RHII 0x40000
/** HMB Restrict Non-Operational Power State Access (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 17 of 'ctratt' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_CTRATT_HMBR 0x20000
/** MDTS and Size Limits Exclude Metadata (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 16 of 'ctratt' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_CTRATT_MEM 0x10000
/** Extended LBA Formats Supported (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 15 of 'ctratt' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_CTRATT_ELBAS 0x8000
/** Delete NVM Set (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 14 of 'ctratt' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_CTRATT_DNVMS 0x4000
/** Delete Endurance Group (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 13 of 'ctratt' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_CTRATT_DEG 0x2000
/** Variable Capacity Management (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 12 of 'ctratt' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_CTRATT_VCM 0x1000
/** Fixed Capacity Management (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 11 of 'ctratt' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_CTRATT_FCM 0x800
/** Multi-Domain Subsystem (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 10 of 'ctratt' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_CTRATT_MDS 0x400
/** UUID List (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 9 of 'ctratt' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_CTRATT_ULIST 0x200
/** SQ Associations (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 8 of 'ctratt' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_CTRATT_SQA 0x100
/** Namespace Granularity (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 7 of 'ctratt' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_CTRATT_NG 0x80
/** Traffic Based Keep Alive Support (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 6 of 'ctratt' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_CTRATT_TBKAS 0x40
/** Predictable Latency Mode (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 5 of 'ctratt' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_CTRATT_PLM 0x20
/** Endurance Groups (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 4 of 'ctratt' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_CTRATT_EGS 0x10
/** Read Recovery Levels (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 3 of 'ctratt' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_CTRATT_RRLVLS 0x8
/** NVM Sets (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 2 of 'ctratt' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_CTRATT_NSETS 0x4
/** Non-Operational Power State Permissive Mode (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 1 of 'ctratt' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_CTRATT_NOPSPM 0x2
/** Host Identifier Support (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 0 of 'ctratt' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_CTRATT_HIDS 0x1
/** Read Recovery Level 15 (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 15 of 'rrls' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_RRLS_RRL15 0x8000
/** Read Recovery Level 14 (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 14 of 'rrls' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_RRLS_RRL14 0x4000
/** Read Recovery Level 13 (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 13 of 'rrls' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_RRLS_RRL13 0x2000
/** Read Recovery Level 12 (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 12 of 'rrls' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_RRLS_RRL12 0x1000
/** Read Recovery Level 11 (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 11 of 'rrls' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_RRLS_RRL11 0x800
/** Read Recovery Level 10 (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 10 of 'rrls' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_RRLS_RRL10 0x400
/** Read Recovery Level 9 (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 9 of 'rrls' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_RRLS_RRL9 0x200
/** Read Recovery Level 8 (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 8 of 'rrls' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_RRLS_RRL8 0x100
/** Read Recovery Level 7 (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 7 of 'rrls' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_RRLS_RRL7 0x80
/** Read Recovery Level 6 (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 6 of 'rrls' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_RRLS_RRL6 0x40
/** Read Recovery Level 5 (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 5 of 'rrls' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_RRLS_RRL5 0x20
/** Read Recovery Level 4 (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 4 of 'rrls' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_RRLS_RRL4 0x10
/** Read Recovery Level 3 (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 3 of 'rrls' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_RRLS_RRL3 0x8
/** Read Recovery Level 2 (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 2 of 'rrls' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_RRLS_RRL2 0x4
/** Read Recovery Level 1 (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 1 of 'rrls' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_RRLS_RRL1 0x2
/** Read Recovery Level 0 (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 0 of 'rrls' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_RRLS_RRL0 0x1
/** Set Features Boot Partition Write Protection Support (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 2 of 'bpcap' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_BPCAP_SFBPWPS 0x4
/** RPMB Boot Partition Write Protection Support (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bits 1:0 of 'bpcap' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_BPCAP_RPMBBPWPS 0x3
/** CXL HDM Support (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 0 of 'chsi' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_CHSI_CHS 0x1
/** PLS Forced Quiescence (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 1 of 'plsi' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_PLSI_PLSFQ 0x2
/** PLS Emergency Power Fail (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 0 of 'plsi' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_PLSI_PLSEPF 0x1
/** Reachability Group ID Changeable (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 1 of 'crcap' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_CRCAP_RGIDC 0x2
/** Reachability Reporting Supported (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 0 of 'crcap' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_CRCAP_RRSUP 0x1
/** NVMe Enclosure (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 1 of 'nvmsr' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_NVMSR_NVMEE 0x2
/** NVMe Storage Device (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 0 of 'nvmsr' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_NVMSR_NVMESD 0x1
/** VPD Write Cycles Remaining Valid (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 7 of 'vwci' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_VWCI_VWCRV 0x80
/** VPD Write Cycles Remaining (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bits 6:0 of 'vwci' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_VWCI_VWCR 0x7f
/** PCIe Port Management Endpoint (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 1 of 'mec' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_MEC_PCIEME 0x2
/** 2-Wire Port Management Endpoint (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 0 of 'mec' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_MEC_TWPME 0x1
/** Controller-scoped Command and Feature Lockdown Supported (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 13 of 'oacs' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_OACS_CCFLS 0x2000
/** Exported NVM Subsystem Support (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 12 of 'oacs' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_OACS_ENSS 0x1000
/** Host Managed Live Migration Support (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 11 of 'oacs' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_OACS_HMLMS 0x800
/** Command and Feature Lockdown Supported (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 10 of 'oacs' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_OACS_CFLS 0x400
/** Get LBA Status Supported (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 9 of 'oacs' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_OACS_GLSS 0x200
/** Doorbell Buffer Config Supported (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 8 of 'oacs' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_OACS_DBCS 0x100
/** Virtualization Management Supported (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 7 of 'oacs' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_OACS_VMS 0x80
/** NVMe-MI Send Receive Supported (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 6 of 'oacs' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_OACS_NSRS 0x40
/** Directives Supported (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 5 of 'oacs' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_OACS_DIRS 0x20
/** Device Self-test Supported (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 4 of 'oacs' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_OACS_DSTS 0x10
/** Namespace Management Supported (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 3 of 'oacs' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_OACS_NMS 0x8
/** Firmware Download Supported (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 2 of 'oacs' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_OACS_FWDS 0x4
/** Format NVM Supported (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 1 of 'oacs' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_OACS_FNVMS 0x2
/** Security Send Receive Supported (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 0 of 'oacs' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_OACS_SSRS 0x1
/** Support Multiple Update Detection (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 5 of 'frmw' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_FRMW_SMUD 0x20
/** Firmware Activation Without Reset (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 4 of 'frmw' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_FRMW_FAWR 0x10
/** Number Of Firmware Slots (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bits 3:1 of 'frmw' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_FRMW_NOFS 0xe
/** First Firmware Slot Read Only (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 0 of 'frmw' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_FRMW_FFSRO 0x1
/** Data Area 4 Support (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 6 of 'lpa' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_LPA_DA4S 0x40
/** Miscellaneous Log Page Support (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 5 of 'lpa' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_LPA_MLPS 0x20
/** Persistent Event Support (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 4 of 'lpa' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_LPA_PES 0x10
/** Telemetry Support (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 3 of 'lpa' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_LPA_TS 0x8
/** Log Page Extended Data Support (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 2 of 'lpa' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_LPA_LPEDS 0x4
/** Commands Supported and Effects Support (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 1 of 'lpa' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_LPA_CSES 0x2
/** SMART Support (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 0 of 'lpa' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_LPA_SMARTS 0x1
/** Vendor Specific Command Format (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 0 of 'avscc' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_AVSCC_VSCF 0x1
/** Autonomous Power Transition Support (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 0 of 'apsta' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_APSTA_APTS 0x1
/** Access Size (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bits 31:24 of 'rpmbs' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_RPMBS_ASZE 0xff000000
/** Total Size (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bits 23:16 of 'rpmbs' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_RPMBS_TSZE 0xff0000
/** Authentication Method (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bits 5:3 of 'rpmbs' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_RPMBS_AUTHM 0x38
/** Number of RPMB Units (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bits 2:0 of 'rpmbs' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_RPMBS_NRPMBU 0x7
/** Host-Initiated Refresh Support (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 1 of 'dsto' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_DSTO_HIRS 0x2
/** Single Device Self-test Operation (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 0 of 'dsto' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_DSTO_SDSO 0x1
/** Host Controlled Thermal Management Support (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 0 of 'hctma' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_HCTMA_HCTMS 0x1
/** No-Deallocate Modifies Media After Sanitize (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bits 31:30 of 'sanicap' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_SANICAP_NODMMAS 0xc0000000
/** No-Deallocate Inhibited (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 29 of 'sanicap' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_SANICAP_NDI 0x20000000
/** Sanitize Purge Request and Reporting Supported (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 5 of 'sanicap' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_SANICAP_SPRRS 0x20
/** Namespace Verification Support (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 4 of 'sanicap' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_SANICAP_NVERS 0x10
/** Verification Support (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 3 of 'sanicap' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_SANICAP_VERS 0x8
/** Overwrite Support (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 2 of 'sanicap' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_SANICAP_OWS 0x4
/** Block Erase Support (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 1 of 'sanicap' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_SANICAP_BES 0x2
/** Crypto Erase Support (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 0 of 'sanicap' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_SANICAP_CES 0x1
/** ANA Group ID Support (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 7 of 'anacap' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_ANACAP_ANAGIDS 0x80
/** ANA Group ID Locked When Attached Support (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 6 of 'anacap' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_ANACAP_ANAGIDLWAS 0x40
/** Report ANA Change State (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 4 of 'anacap' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_ANACAP_RANACS 0x10
/** Report ANA Persistent Loss State (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 3 of 'anacap' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_ANACAP_RANAPLS 0x8
/** Report ANA Inaccessible State (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 2 of 'anacap' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_ANACAP_RANAIS 0x4
/** Report ANA Non-Optimized State (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 1 of 'anacap' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_ANACAP_RANANOS 0x2
/** Report ANA Optimized State (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 0 of 'anacap' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_ANACAP_RANAOS 0x1
/** Key Per I/O Scope (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 1 of 'kpioc' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_KPIOC_KPIOSC 0x2
/** Key Per I/O Supported (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 0 of 'kpioc' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_KPIOC_KPIOS 0x1
/** Restore Default Capacity Management Configuration Supported (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 2 of 'rmdca' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_RMDCA_RDCCS 0x4
/** Restore Default Namespace Configuration Supported (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 1 of 'rmdca' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_RMDCA_RDNCS 0x2
/** Restore Default NVM Subsystem Configuration Supported (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 0 of 'rmdca' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_RMDCA_RDSCS 0x1
/** Temperature Threshold Maximum Hysteresis (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bits 2:0 of 'tmpthha' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_TMPTHHA_TMPTHMH 0x7
/** Maximum Unlimited Power Scale (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bits 1:0 of 'mupa' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_MUPA_MUPS 0x3
/** CDP Authentication Algorithm (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bits 7:0 of 'cdpa' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_CDPA_CDPALG 0xff
/** Sample Rate Scale (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bits 15:8 of 'ipmsr' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_IPMSR_SRS 0xff00
/** Sample Rate Value (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bits 7:0 of 'ipmsr' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_IPMSR_SRV 0xff
/** Exported NVM Subsystem Support Migration Support (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 1 of 'ensa' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_ENSA_ENSMS 0x2
/** Exported NVM Subsystem Template Support (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 0 of 'ensa' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_ENSA_ENSTS 0x1
/** Exported Namespace Format 1 (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 1 of 'endsfs' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_ENDSFS_ENF1 0x2
/** Exported Namespace Format 0 (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 0 of 'endsfs' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_ENDSFS_ENF0 0x1
/** Interval Voltage Measurement Sample Rate (Base 2.4, 5.2.14.2.1, Figure 339, page 381, PDF page 409); bits 31:16 of 'Voltage Sensor' */
#define XNVME_SPEC_BASE_VOLTAGE_SENSOR_IVMSR 0xffff0000
/** Voltage Sample Scale (Base 2.4, 5.2.14.2.1, Figure 339, page 381, PDF page 409); bits 15:14 of 'Voltage Sensor' */
#define XNVME_SPEC_BASE_VOLTAGE_SENSOR_VOLSS 0xc000
/** Power Input Type (Base 2.4, 5.2.14.2.1, Figure 339, page 381, PDF page 409); bits 13:0 of 'Voltage Sensor' */
#define XNVME_SPEC_BASE_VOLTAGE_SENSOR_PIT 0x3fff
/** Maximum I/O Submission Queue Entry Size (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bits 7:4 of 'sqes' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_SQES_MAXSQES 0xf0
/** Minimum I/O Submission Queue Entry Size (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bits 3:0 of 'sqes' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_SQES_MINSQES 0xf
/** Maximum I/O Completion Queue Entry Size (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bits 7:4 of 'cqes' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_CQES_MAXCQES 0xf0
/** Minimum I/O Completion Queue Entry Size (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bits 3:0 of 'cqes' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_CQES_MINCQES 0xf
/** Namespace Zeroes Support (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 12 of 'oncs' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_ONCS_NSZS 0x1000
/** Maximum Write Zeroes with Deallocate (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 11 of 'oncs' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_ONCS_MAXWZD 0x800
/** NVM All Fast Copy (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 10 of 'oncs' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_ONCS_NVMAFC 0x400
/** NVM Copy Single Atomicity (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 9 of 'oncs' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_ONCS_NVMCSA 0x200
/** Copy Support (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 8 of 'oncs' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_ONCS_NVMCPYS 0x100
/** Verify Support (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 7 of 'oncs' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_ONCS_NVMVFYS 0x80
/** Timestamp Support (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 6 of 'oncs' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_ONCS_TSS 0x40
/** Reservations Support (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 5 of 'oncs' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_ONCS_RESERVS 0x20
/** Save and Select Feature Support (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 4 of 'oncs' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_ONCS_SSFS 0x10
/** Write Zeroes Support Variants (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 3 of 'oncs' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_ONCS_NVMWZSV 0x8
/** Dataset Management Support Variants (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 2 of 'oncs' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_ONCS_NVMDSMSV 0x4
/** Write Uncorrectable Support Variants (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 1 of 'oncs' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_ONCS_NVMWUSV 0x2
/** Compare Command Support (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 0 of 'oncs' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_ONCS_NVMCMPS 0x1
/** Fused Compare and Write Supported (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 0 of 'fuses' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_FUSES_FCWS 0x1
/** Format NVM Broadcast Support (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 3 of 'fna' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_FNA_FNVMBS 0x8
/** Cryptographic Erase Supported (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 2 of 'fna' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_FNA_CRYES 0x4
/** Secure Erase Namespace Scope (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 1 of 'fna' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_FNA_SENS 0x2
/** Format Namespace Scope (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 0 of 'fna' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_FNA_FNS 0x1
/** Flush Behavior (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bits 2:1 of 'vwc' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_VWC_FB 0x6
/** Volatile Write Cache Present (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 0 of 'vwc' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_VWC_VWCP 0x1
/** Same NVM Vendor Specific Command Format (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 0 of 'icsvscc' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_ICSVSCC_SNVSCF 0x1
/** Permanent Write Protect Support (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 2 of 'nwpc' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_NWPC_PWPS 0x4
/** Write Protect Until Power Cycle Support (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 1 of 'nwpc' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_NWPC_WPUPCS 0x2
/** No Write Protect and Write Protect Support (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 0 of 'nwpc' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_NWPC_NWPWPS 0x1
/** Copy Descriptor Format 4 Support (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 4 of 'cdfs' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_CDFS_CDF4S 0x10
/** Copy Descriptor Format 3 Support (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 3 of 'cdfs' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_CDFS_CDF3S 0x8
/** Copy Descriptor Format 2 Support (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 2 of 'cdfs' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_CDFS_CDF2S 0x4
/** Copy Descriptor Format 1 Support (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 1 of 'cdfs' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_CDFS_CDF1S 0x2
/** Copy Descriptor Format 0 Support (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 0 of 'cdfs' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_CDFS_CDF0S 0x1
/** Transport SGL Data Block Descriptor Support (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 21 of 'sgls' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_SGLS_TSDBDS 0x200000
/** SGL Address Offset Supported (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 20 of 'sgls' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_SGLS_SAOS 0x100000
/** MPTR SGL Descriptor Support (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 19 of 'sgls' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_SGLS_MSDS 0x80000
/** Length Larger than Data Transfer Support (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 18 of 'sgls' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_SGLS_LLDTS 0x40000
/** Metadata Buffer Alignment (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 17 of 'sgls' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_SGLS_MBA 0x20000
/** SGL Bit Bucket Descriptor Supported (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 16 of 'sgls' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_SGLS_SBBDS 0x10000
/** SGL Descriptor Threshold (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bits 15:8 of 'sgls' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_SGLS_SDT 0xff00
/** Keyed SGL Data Block Descriptor Support (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 2 of 'sgls' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_SGLS_KSDBDS 0x4
/** Memory Range Tracking Length Limit (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 2 of 'trattr' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_TRATTR_MRTLL 0x4
/** Track User Data Changes Support (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 1 of 'trattr' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_TRATTR_TUDCS 0x2
/** Track Host Memory Changes Support (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 0 of 'trattr' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_TRATTR_THMCS 0x1
/** Non-Zero NVM Set ID Support (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 1 of 'fcatt' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_FCATT_NZNSETIDS 0x2
/** Dynamic Controller Model Support (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 0 of 'fcatt' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_FCATT_DCMS 0x1
/** Disconnect Command Support (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366); bit 0 of 'ofcs' */
#define XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_OFCS_DCS 0x1

/**
 * The values of Idle I/O Exit Latency Limit Support and Scope (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366)
 */
enum xnvme_spec_base_identify_controller_ctratt_iiellss {
	XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_CTRATT_IIELLSS_NOT_SUPPORTED = 0x0, ///< Not Supported (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366)
	XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_CTRATT_IIELLSS_POWER_STATE = 0x1, ///< Power State (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366)
	XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_CTRATT_IIELLSS_GLOBAL = 0x2, ///< Global (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366)
};

/**
 * The values of Reservations and Host Identifier Interaction (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366)
 */
enum xnvme_spec_base_identify_controller_ctratt_rhii {
	XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_CTRATT_RHII_NOT_REPORTED = 0x0, ///< Not Reported (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366)
};

/**
 * The values of Controller Type (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366)
 */
enum xnvme_spec_base_identify_controller_cntrltype {
	XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_CNTRLTYPE_IO_CONTROLLER = 0x1, ///< I/O controller (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366)
	XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_CNTRLTYPE_DISCOVERY_CONTROLLER = 0x2, ///< Discovery controller (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366)
	XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_CNTRLTYPE_ADMINISTRATIVE_CONTROLLER = 0x3, ///< Administrative controller (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366)
};

/**
 * The values of Authentication Method (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366)
 */
enum xnvme_spec_base_identify_controller_rpmbs_authm {
	XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_RPMBS_AUTHM_HMAC_SHA_256 = 0x0, ///< HMAC SHA-256 (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366)
};

/**
 * The values of Maximum Unlimited Power Scale (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366)
 */
enum xnvme_spec_base_identify_controller_mupa_mups {
	XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_MUPA_MUPS_0_0001_W = 0x1, ///< 0.0001 W (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366)
	XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_MUPA_MUPS_0_01_W = 0x2, ///< 0.01 W (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366)
};

/**
 * The values of Voltage Sample Scale (Base 2.4, 5.2.14.2.1, Figure 339, page 381, PDF page 409)
 */
enum xnvme_spec_base_voltage_sensor_volss {
	XNVME_SPEC_BASE_VOLTAGE_SENSOR_VOLSS_0_001_V = 0x1, ///< 0.001 V (Base 2.4, 5.2.14.2.1, Figure 339, page 381, PDF page 409)
	XNVME_SPEC_BASE_VOLTAGE_SENSOR_VOLSS_0_01_V = 0x2, ///< 0.01 V (Base 2.4, 5.2.14.2.1, Figure 339, page 381, PDF page 409)
};

/**
 * The values of Discovery Controller Type (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366)
 */
enum xnvme_spec_base_identify_controller_dctype {
	XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_DCTYPE_DDC = 0x1, ///< DDC (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366)
	XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_DCTYPE_CDC = 0x2, ///< CDC (Base 2.4, 5.2.14.2.1, Figure 338, page 338, PDF page 366)
};

/**
 * Identify - Namespace Identification Descriptor (Base 2.4, 5.2.14.2.3, Figure 342, page 386, PDF page 414)
 *
 * Followed, from byte 4, by as many Namespace Identifier as the data says
 */
struct __attribute__((packed)) xnvme_spec_base_namespace_identification_descriptor {
	uint8_t nidt; ///< Namespace Identifier Type, byte 0
	uint8_t nidl; ///< Namespace Identifier Length, byte 1
	uint16_t rsvd2; ///< Reserved, bytes 3:2
};

/**
 * The values of Namespace Identifier Type (Base 2.4, 5.2.14.2.3, Figure 342, page 386, PDF page 414)
 */
enum xnvme_spec_base_namespace_identification_descriptor_nidt {
	XNVME_SPEC_BASE_NAMESPACE_IDENTIFICATION_DESCRIPTOR_NIDT_IEEE_EXTENDED_UNIQUE_IDENTIFIER = 0x1, ///< IEEE Extended Unique Identifier (Base 2.4, 5.2.14.2.3, Figure 342, page 386, PDF page 414)
	XNVME_SPEC_BASE_NAMESPACE_IDENTIFICATION_DESCRIPTOR_NIDT_NAMESPACE_GLOBALLY_UNIQUE_IDENTIFIER = 0x2, ///< Namespace Globally Unique Identifier (Base 2.4, 5.2.14.2.3, Figure 342, page 386, PDF page 414)
	XNVME_SPEC_BASE_NAMESPACE_IDENTIFICATION_DESCRIPTOR_NIDT_NAMESPACE_UUID = 0x3, ///< Namespace UUID (Base 2.4, 5.2.14.2.3, Figure 342, page 386, PDF page 414)
	XNVME_SPEC_BASE_NAMESPACE_IDENTIFICATION_DESCRIPTOR_NIDT_COMMAND_SET_IDENTIFIER = 0x4, ///< Command Set Identifier (Base 2.4, 5.2.14.2.3, Figure 342, page 386, PDF page 414)
};

/**
 * NVM Set Attributes Entry (Base 2.4, 5.2.14.2.4, Figure 345, page 387, PDF page 415)
 */
struct __attribute__((packed)) xnvme_spec_base_nvm_set_attributes_entry {
	uint16_t nsetid; ///< NVM Set Identifier, bytes 1:0
	uint16_t endgid; ///< Endurance Group Identifier, bytes 3:2
	uint32_t rsvd4; ///< Reserved, bytes 7:4
	uint32_t r4krt; ///< Random 4 KiB Read Typical, bytes 11:8
	uint32_t ows; ///< Optimal Write Size, bytes 15:12
	uint8_t tnvmsc[16]; ///< Total NVM Set Capacity, bytes 31:16
	uint8_t unvmsc[16]; ///< Unallocated NVM Set Capacity, bytes 47:32
	uint8_t rsvd48[80]; ///< Reserved, bytes 127:48
};

/**
 * NVM Set List (Base 2.4, 5.2.14.2.4, Figure 344, page 387, PDF page 415)
 *
 * Followed, from byte 128, by as many Entry as the data says, each a 'struct xnvme_spec_base_nvm_set_attributes_entry'
 */
struct __attribute__((packed)) xnvme_spec_base_nvm_set_list {
	uint8_t nument; ///< Number of Entries, byte 0
	uint8_t rsvd1[127]; ///< Reserved, bytes 127:1
};

/**
 * Identify - I/O Command Set Independent Identify Namespace (Base 2.4, 5.2.14.2.8, Figure 346, page 389, PDF page 417)
 */
struct __attribute__((packed)) xnvme_spec_base_io_command_set_independent_identify_namespace {
	uint8_t nsfeat; ///< Common Namespace Features, byte 0, see XNVME_SPEC_BASE_IO_COMMAND_SET_INDEPENDENT_IDENTIFY_NAMESPACE_NSFEAT_*
	uint8_t nmic; ///< Namespace Multi-path I/O and Namespace Sharing Capabilities, byte 1, see XNVME_SPEC_BASE_IO_COMMAND_SET_INDEPENDENT_IDENTIFY_NAMESPACE_NMIC_*
	uint8_t rescap; ///< Reservation Capabilities, byte 2, see XNVME_SPEC_BASE_IO_COMMAND_SET_INDEPENDENT_IDENTIFY_NAMESPACE_RESCAP_*
	uint8_t fpi; ///< Format Progress Indicator, byte 3, see XNVME_SPEC_BASE_IO_COMMAND_SET_INDEPENDENT_IDENTIFY_NAMESPACE_FPI_*
	uint32_t anagrpid; ///< ANA Group Identifier, bytes 7:4
	uint8_t nsattr; ///< Namespace Attributes, byte 8, see XNVME_SPEC_BASE_IO_COMMAND_SET_INDEPENDENT_IDENTIFY_NAMESPACE_NSATTR_*
	uint8_t rsvd9; ///< Reserved, byte 9
	uint16_t nvmsetid; ///< NVM Set Identifier, bytes 11:10
	uint16_t endgid; ///< Endurance Group Identifier, bytes 13:12
	uint8_t nstat; ///< Namespace Status, byte 14, see XNVME_SPEC_BASE_IO_COMMAND_SET_INDEPENDENT_IDENTIFY_NAMESPACE_NSTAT_*
	uint8_t kpios; ///< Key Per I/O Status, byte 15, see XNVME_SPEC_BASE_IO_COMMAND_SET_INDEPENDENT_IDENTIFY_NAMESPACE_KPIOS_*
	uint16_t maxkt; ///< Maximum Key Tag, bytes 17:16
	uint16_t rsvd18; ///< Reserved, bytes 19:18
	uint32_t rgrpid; ///< Reachability Group Identifier, bytes 23:20
	uint8_t rsvd24[4072]; ///< Reserved, bytes 4095:24
};

/** Volatile Write Cache Not Present (Base 2.4, 5.2.14.2.8, Figure 346, page 389, PDF page 417); bit 5 of 'nsfeat' */
#define XNVME_SPEC_BASE_IO_COMMAND_SET_INDEPENDENT_IDENTIFY_NAMESPACE_NSFEAT_VWCNP 0x20
/** Rotational Media (Base 2.4, 5.2.14.2.8, Figure 346, page 389, PDF page 417); bit 4 of 'nsfeat' */
#define XNVME_SPEC_BASE_IO_COMMAND_SET_INDEPENDENT_IDENTIFY_NAMESPACE_NSFEAT_RMEDIA 0x10
/** UID Reuse (Base 2.4, 5.2.14.2.8, Figure 346, page 389, PDF page 417); bit 3 of 'nsfeat' */
#define XNVME_SPEC_BASE_IO_COMMAND_SET_INDEPENDENT_IDENTIFY_NAMESPACE_NSFEAT_UIDREUSE 0x8
/** Dispersed Namespace (Base 2.4, 5.2.14.2.8, Figure 346, page 389, PDF page 417); bit 1 of 'nmic' */
#define XNVME_SPEC_BASE_IO_COMMAND_SET_INDEPENDENT_IDENTIFY_NAMESPACE_NMIC_DISNS 0x2
/** Shared Namespace (Base 2.4, 5.2.14.2.8, Figure 346, page 389, PDF page 417); bit 0 of 'nmic' */
#define XNVME_SPEC_BASE_IO_COMMAND_SET_INDEPENDENT_IDENTIFY_NAMESPACE_NMIC_SHRNS 0x1
/** Ignore Existing Key Support (Base 2.4, 5.2.14.2.8, Figure 346, page 389, PDF page 417); bit 7 of 'rescap' */
#define XNVME_SPEC_BASE_IO_COMMAND_SET_INDEPENDENT_IDENTIFY_NAMESPACE_RESCAP_IEKS 0x80
/** Exclusive Access - All Registrants Support (Base 2.4, 5.2.14.2.8, Figure 346, page 389, PDF page 417); bit 6 of 'rescap' */
#define XNVME_SPEC_BASE_IO_COMMAND_SET_INDEPENDENT_IDENTIFY_NAMESPACE_RESCAP_EAARS 0x40
/** Write Exclusive - All Registrants Support (Base 2.4, 5.2.14.2.8, Figure 346, page 389, PDF page 417); bit 5 of 'rescap' */
#define XNVME_SPEC_BASE_IO_COMMAND_SET_INDEPENDENT_IDENTIFY_NAMESPACE_RESCAP_WEARS 0x20
/** Exclusive Access - Registrants Only Support (Base 2.4, 5.2.14.2.8, Figure 346, page 389, PDF page 417); bit 4 of 'rescap' */
#define XNVME_SPEC_BASE_IO_COMMAND_SET_INDEPENDENT_IDENTIFY_NAMESPACE_RESCAP_EAROS 0x10
/** Write Exclusive - Registrants Only Support (Base 2.4, 5.2.14.2.8, Figure 346, page 389, PDF page 417); bit 3 of 'rescap' */
#define XNVME_SPEC_BASE_IO_COMMAND_SET_INDEPENDENT_IDENTIFY_NAMESPACE_RESCAP_WEROS 0x8
/** Exclusive Access Support (Base 2.4, 5.2.14.2.8, Figure 346, page 389, PDF page 417); bit 2 of 'rescap' */
#define XNVME_SPEC_BASE_IO_COMMAND_SET_INDEPENDENT_IDENTIFY_NAMESPACE_RESCAP_EAS 0x4
/** Write Exclusive Support (Base 2.4, 5.2.14.2.8, Figure 346, page 389, PDF page 417); bit 1 of 'rescap' */
#define XNVME_SPEC_BASE_IO_COMMAND_SET_INDEPENDENT_IDENTIFY_NAMESPACE_RESCAP_WES 0x2
/** Persist Through Power Loss Support (Base 2.4, 5.2.14.2.8, Figure 346, page 389, PDF page 417); bit 0 of 'rescap' */
#define XNVME_SPEC_BASE_IO_COMMAND_SET_INDEPENDENT_IDENTIFY_NAMESPACE_RESCAP_PTPLS 0x1
/** Format Progress Indicator Support (Base 2.4, 5.2.14.2.8, Figure 346, page 389, PDF page 417); bit 7 of 'fpi' */
#define XNVME_SPEC_BASE_IO_COMMAND_SET_INDEPENDENT_IDENTIFY_NAMESPACE_FPI_FPIS 0x80
/** Remaining Format NVM (Base 2.4, 5.2.14.2.8, Figure 346, page 389, PDF page 417); bits 6:0 of 'fpi' */
#define XNVME_SPEC_BASE_IO_COMMAND_SET_INDEPENDENT_IDENTIFY_NAMESPACE_FPI_RFNVM 0x7f
/** Currently Write Protected (Base 2.4, 5.2.14.2.8, Figure 346, page 389, PDF page 417); bit 0 of 'nsattr' */
#define XNVME_SPEC_BASE_IO_COMMAND_SET_INDEPENDENT_IDENTIFY_NAMESPACE_NSATTR_CWP 0x1
/** I/O Impacted (Base 2.4, 5.2.14.2.8, Figure 346, page 389, PDF page 417); bits 2:1 of 'nstat' */
#define XNVME_SPEC_BASE_IO_COMMAND_SET_INDEPENDENT_IDENTIFY_NAMESPACE_NSTAT_IOI 0x6
/** Namespace Ready (Base 2.4, 5.2.14.2.8, Figure 346, page 389, PDF page 417); bit 0 of 'nstat' */
#define XNVME_SPEC_BASE_IO_COMMAND_SET_INDEPENDENT_IDENTIFY_NAMESPACE_NSTAT_NRDY 0x1
/** Key Per I/O Supported in Namespace (Base 2.4, 5.2.14.2.8, Figure 346, page 389, PDF page 417); bit 1 of 'kpios' */
#define XNVME_SPEC_BASE_IO_COMMAND_SET_INDEPENDENT_IDENTIFY_NAMESPACE_KPIOS_KPIOSNS 0x2
/** Key Per I/O Enabled in Namespace (Base 2.4, 5.2.14.2.8, Figure 346, page 389, PDF page 417); bit 0 of 'kpios' */
#define XNVME_SPEC_BASE_IO_COMMAND_SET_INDEPENDENT_IDENTIFY_NAMESPACE_KPIOS_KPIOENS 0x1

/**
 * UUID List Entry (Base 2.4, 5.2.14.2.14, Figure 348, page 394, PDF page 422)
 */
struct __attribute__((packed)) xnvme_spec_base_uuid_list_entry {
	uint8_t rsvd0; ///< Reserved, byte 0
	uint8_t rsvd1[15]; ///< Reserved, bytes 15:1
	uint8_t uuid[16]; ///< Universally Unique Identifier, bytes 31:16
};

/**
 * UUID List (Base 2.4, 5.2.14.2.14, Figure 347, page 394, PDF page 422)
 */
struct __attribute__((packed)) xnvme_spec_base_uuid_list {
	uint8_t rsvd0[32]; ///< Reserved, bytes 31:0
	struct xnvme_spec_base_uuid_list_entry uuid[127]; ///< UUID, 127 of 32 bytes, bytes 4095:32
};

/**
 * Domain Attributes Entry (Base 2.4, 5.2.14.2.15, Figure 351, page 395, PDF page 423)
 */
struct __attribute__((packed)) xnvme_spec_base_domain_attributes_entry {
	uint16_t did; ///< Domain Identifier, bytes 1:0
	uint8_t rsvd2[14]; ///< Reserved, bytes 15:2
	uint8_t tdc[16]; ///< Total Domain Capacity, bytes 31:16
	uint8_t udc[16]; ///< Unallocated Domain Capacity, bytes 47:32
	uint8_t megdc[16]; ///< Max Endurance Group Domain Capacity, bytes 63:48
	uint8_t rsvd64[64]; ///< Reserved, bytes 127:64
};

/**
 * Domain List (Base 2.4, 5.2.14.2.15, Figure 350, page 394, PDF page 422)
 *
 * Followed, from byte 128, by as many Entry as the data says, each a 'struct xnvme_spec_base_domain_attributes_entry'
 */
struct __attribute__((packed)) xnvme_spec_base_domain_list {
	uint8_t nument; ///< Number of Entries, byte 0
	uint8_t rsvd1[127]; ///< Reserved, bytes 127:1
};

/**
 * Endurance Group List (Base 2.4, 5.2.14.2.16, Figure 353, page 395, PDF page 423)
 *
 * Followed, from byte 2, by as many Identifier as the data says
 */
struct __attribute__((packed)) xnvme_spec_base_endurance_group_list {
	uint16_t negids; ///< Number of Endurance Group Identifiers, bytes 1:0
};

/**
 * Identify I/O Command Set (Base 2.4, 5.2.14.2.19, Figure 354, page 396, PDF page 424)
 */
struct __attribute__((packed)) xnvme_spec_base_identify_io_command_set {
	uint64_t iocsc[512]; ///< I/O Command Set Combination, 512 of 8 bytes of XNVME_SPEC_BASE_IO_COMMAND_SET_VECTOR_*, bytes 4095:0
};

/** Computational Programs Namespace Command Set (Base 2.4, 5.2.14.2.19, Figure 355, page 397, PDF page 425); bit 4 of 'I/O Command Set Vector' */
#define XNVME_SPEC_BASE_IO_COMMAND_SET_VECTOR_CPNCS 0x10
/** Subsystem Local Memory Command Set (Base 2.4, 5.2.14.2.19, Figure 355, page 397, PDF page 425); bit 3 of 'I/O Command Set Vector' */
#define XNVME_SPEC_BASE_IO_COMMAND_SET_VECTOR_SLMCS 0x8
/** Zoned Namespace Command Set (Base 2.4, 5.2.14.2.19, Figure 355, page 397, PDF page 425); bit 2 of 'I/O Command Set Vector' */
#define XNVME_SPEC_BASE_IO_COMMAND_SET_VECTOR_ZNSCS 0x4
/** Key Value Command Set (Base 2.4, 5.2.14.2.19, Figure 355, page 397, PDF page 425); bit 1 of 'I/O Command Set Vector' */
#define XNVME_SPEC_BASE_IO_COMMAND_SET_VECTOR_KVCS 0x2
/** NVM Command Set (Base 2.4, 5.2.14.2.19, Figure 355, page 397, PDF page 425); bit 0 of 'I/O Command Set Vector' */
#define XNVME_SPEC_BASE_IO_COMMAND_SET_VECTOR_NVMCS 0x1

/**
 * Underlying Namespace Entry (Base 2.4, 5.2.14.2.20, Figure 357, page 398, PDF page 426)
 */
struct __attribute__((packed)) xnvme_spec_base_underlying_namespace_entry {
	uint8_t unsnqn[256]; ///< Underlying NVM Subsystem NQN, bytes 255:0
	uint32_t nsid; ///< Namespace Identifier, bytes 259:256
	uint16_t cntlid; ///< Controller ID, bytes 261:260
	uint8_t rsvd262[56]; ///< Reserved, bytes 317:262
	uint16_t idx; ///< Index, bytes 319:318
};

/**
 * Underlying Namespace List (Base 2.4, 5.2.14.2.20, Figure 356, page 397, PDF page 425)
 *
 * Followed, from byte 16, by as many Underlying Namespace Entry as the data says, each a 'struct xnvme_spec_base_underlying_namespace_entry'
 */
struct __attribute__((packed)) xnvme_spec_base_underlying_namespace_list {
	uint64_t genctr; ///< Generation Counter, bytes 7:0
	uint64_t nument; ///< Number Entries, bytes 15:8
};

/**
 * Supported Controller State Formats (Base 2.4, 5.2.14.2.22, Figure 358, page 398, PDF page 426)
 *
 * Followed, from byte 2, by as many NVMe Controller State Data Structure Version as the data says
 *
 * Not members:
 * Vendor Specific Controller State UUID 1: at an offset the data gives, in 'nvme_controller_state_data_structure_version'
 * Vendor Specific Controller State UUID 2: at an offset the data gives, in 'nvme_controller_state_data_structure_version'
 * Vendor Specific Controller State UUID NUUID: at an offset the data gives, in 'nvme_controller_state_data_structure_version'
 */
struct __attribute__((packed)) xnvme_spec_base_supported_controller_state_formats {
	uint8_t nv; ///< Number of Versions, byte 0
	uint8_t nuuid; ///< Number of UUIDs, byte 1
};

/**
 * Exported NVM Subsystem Template UUID List (Base 2.4, 5.2.14.2.23, Figure 359, page 400, PDF page 428)
 */
struct __attribute__((packed)) xnvme_spec_base_exported_nvm_subsystem_template_uuid_list {
	uint8_t rsvd0[16]; ///< Reserved, bytes 15:0
	uint8_t enst[4080]; ///< Exported NVM Subsystem Template, 255 of 16 bytes, bytes 4095:16
};

/**
 * Identify - Primary Controller Capabilities (Base 2.4, 5.2.14.3.1, Figure 360, page 400, PDF page 428)
 */
struct __attribute__((packed)) xnvme_spec_base_primary_controller_capabilities {
	uint16_t cntlid; ///< Controller Identifier, bytes 1:0
	uint16_t portid; ///< Port Identifier, bytes 3:2
	uint8_t crt; ///< Controller Resource Types, byte 4, see XNVME_SPEC_BASE_PRIMARY_CONTROLLER_CAPABILITIES_CRT_*
	uint8_t rsvd5[27]; ///< Reserved, bytes 31:5
	uint32_t vqfrt; ///< VQ Resources Flexible Total, bytes 35:32
	uint32_t vqrfa; ///< VQ Resources Flexible Assigned, bytes 39:36
	uint16_t vqrfap; ///< VQ Resources Flexible Allocated to Primary, bytes 41:40
	uint16_t vqprt; ///< VQ Resources Private Total, bytes 43:42
	uint16_t vqfrsm; ///< VQ Resources Flexible Secondary Maximum, bytes 45:44
	uint16_t vqgran; ///< VQ Flexible Resource Preferred Granularity, bytes 47:46
	uint8_t rsvd48[16]; ///< Reserved, bytes 63:48
	uint32_t vifrt; ///< VI Resources Flexible Total, bytes 67:64
	uint32_t virfa; ///< VI Resources Flexible Assigned, bytes 71:68
	uint16_t virfap; ///< VI Resources Flexible Allocated to Primary, bytes 73:72
	uint16_t viprt; ///< VI Resources Private Total, bytes 75:74
	uint16_t vifrsm; ///< VI Resources Flexible Secondary Maximum, bytes 77:76
	uint16_t vigran; ///< VI Flexible Resource Preferred Granularity, bytes 79:78
	uint8_t rsvd80[4016]; ///< Reserved, bytes 4095:80
};

/** VI Resources Support (Base 2.4, 5.2.14.3.1, Figure 360, page 400, PDF page 428); bit 1 of 'crt' */
#define XNVME_SPEC_BASE_PRIMARY_CONTROLLER_CAPABILITIES_CRT_VIRS 0x2
/** VQ Resources Support (Base 2.4, 5.2.14.3.1, Figure 360, page 400, PDF page 428); bit 0 of 'crt' */
#define XNVME_SPEC_BASE_PRIMARY_CONTROLLER_CAPABILITIES_CRT_VQRS 0x1

/**
 * Secondary Controller List (Base 2.4, 5.2.14.3.2, Figure 361, page 401, PDF page 429)
 */
struct __attribute__((packed)) xnvme_spec_base_secondary_controller_list {
	uint8_t nument; ///< Number of Entries, byte 0
	uint8_t rsvd1[31]; ///< Reserved, bytes 31:1
	uint8_t sc_entry[64]; ///< SC Entry, 2 of 32 bytes, bytes 95:32
};

/**
 * Secondary Controller Entry (Base 2.4, 5.2.14.3.2, Figure 362, page 401, PDF page 429)
 */
struct __attribute__((packed)) xnvme_spec_base_secondary_controller_entry {
	uint16_t scid; ///< Secondary Controller Identifier, bytes 1:0
	uint16_t pcid; ///< Primary Controller Identifier, bytes 3:2
	uint8_t scs; ///< Secondary Controller State, byte 4, see XNVME_SPEC_BASE_SECONDARY_CONTROLLER_ENTRY_SCS_*
	uint8_t rsvd5[3]; ///< Reserved, bytes 7:5
	uint16_t vfn; ///< Virtual Function Number, bytes 9:8
	uint16_t nvq; ///< Number of VQ Flexible Resources Assigned, bytes 11:10
	uint16_t nvi; ///< Number of VI Flexible Resources Assigned, bytes 13:12
	uint8_t rsvd14[18]; ///< Reserved, bytes 31:14
};

/** Online State (Base 2.4, 5.2.14.3.2, Figure 362, page 401, PDF page 429); bit 0 of 'scs' */
#define XNVME_SPEC_BASE_SECONDARY_CONTROLLER_ENTRY_SCS_OLS 0x1

/**
 * Underlying Fabrics Transport Entry (Base 2.4, 5.2.14.4.1, Figure 364, page 403, PDF page 431)
 */
struct __attribute__((packed)) xnvme_spec_base_underlying_fabrics_transport_entry {
	uint8_t traddr[256]; ///< Transport Address, bytes 255:0
	uint8_t tsas[256]; ///< Transport Specific Address Subtype, bytes 511:256
	uint16_t pidup; ///< Port ID of the Underlying Port, bytes 513:512
	uint8_t trtype; ///< Transport Type, byte 514
	uint8_t adrfam; ///< Transport Address Family, byte 515
	uint8_t treq; ///< Transport Requirements, byte 516
	uint8_t rsvd517[59]; ///< Reserved, bytes 575:517
};

/**
 * Ports List (Base 2.4, 5.2.14.4.1, Figure 363, page 402, PDF page 430)
 *
 * Followed, from byte 16, by as many Fabrics Transport Entry as the data says, each a 'struct xnvme_spec_base_underlying_fabrics_transport_entry'
 */
struct __attribute__((packed)) xnvme_spec_base_ports_list {
	uint64_t genctr; ///< Generation Counter, bytes 7:0
	uint64_t nument; ///< Number Entries, bytes 15:8
};

/**
 * Exported NVM Subsystems List (Base 2.4, 5.2.18.1.2, Figure 384, page 414, PDF page 442)
 *
 * Followed, from byte 4, by as many Exported NVM Subsystem Descriptor as the data says
 */
struct __attribute__((packed)) xnvme_spec_base_exported_nvm_subsystems_list {
	uint16_t n; ///< Number of Exported NVM Subsystem Descriptors, bytes 1:0
	uint8_t rsvd2; ///< Reserved, byte 2
	uint8_t gn; ///< Generation Number, byte 3
};

/**
 * Exported NVM Subsystem Descriptor (Base 2.4, 5.2.18.1.2, Figure 385, page 414, PDF page 442)
 */
struct __attribute__((packed)) xnvme_spec_base_exported_nvm_subsystem_descriptor {
	uint8_t esubnqn[256]; ///< Exported NVM Subsystem NQN, bytes 255:0
	uint16_t rsvd256; ///< Reserved, bytes 257:256
	uint16_t esubid; ///< Exported NVM Subsystem Identifier, bytes 259:258
};

/**
 * Exported Namespace Descriptor (Base 2.4, 5.2.18.1.3, Figure 387, page 415, PDF page 443)
 */
struct __attribute__((packed)) xnvme_spec_base_exported_namespace_descriptor {
	uint32_t ensid; ///< Exported Namespace ID, bytes 3:0
	uint32_t unsid; ///< Underlying Namespace ID, bytes 7:4
};

/**
 * Exported Namespace List (Base 2.4, 5.2.18.1.3, Figure 386, page 415, PDF page 443)
 *
 * Followed, from byte 8, by as many Exported Namespace Descriptor as the data says, each a 'struct xnvme_spec_base_exported_namespace_descriptor'
 */
struct __attribute__((packed)) xnvme_spec_base_exported_namespace_list {
	uint16_t n; ///< Number of Exported Namespaces, bytes 1:0
	uint8_t rsvd2; ///< Reserved, byte 2
	uint8_t gn; ///< Generation Number, byte 3
	uint32_t rsvd4; ///< Reserved, bytes 7:4
};

/**
 * Exported Controller Descriptor (Base 2.4, 5.2.18.1.4, Figure 390, page 416, PDF page 444)
 */
struct __attribute__((packed)) xnvme_spec_base_exported_controller_descriptor {
	uint16_t ecntlid; ///< Exported Controller Identifier, bytes 1:0
	uint16_t cntlid; ///< Controller Identifier, bytes 3:2
};

/**
 * Exported Controller List (Base 2.4, 5.2.18.1.4, Figure 389, page 415, PDF page 444)
 *
 * Followed, from byte 4, by as many Exported Controller Descriptor as the data says, each a 'struct xnvme_spec_base_exported_controller_descriptor'
 */
struct __attribute__((packed)) xnvme_spec_base_exported_controller_list {
	uint16_t n; ///< Number of Exported Controllers, bytes 1:0
	uint8_t rsvd2; ///< Reserved, byte 2
	uint8_t gn; ///< Generation Number, byte 3
};

/**
 * Exported Port Descriptor (Base 2.4, 5.2.18.1.5, Figure 393, page 416, PDF page 445)
 */
struct __attribute__((packed)) xnvme_spec_base_exported_port_descriptor {
	uint16_t epid; ///< Exported Port ID, bytes 1:0
	uint16_t pidud; ///< Port ID of the Underlying Port, bytes 3:2
	uint32_t rsvd4; ///< Reserved, bytes 7:4
	uint8_t trsvcid[32]; ///< Transport Service ID, bytes 39:8
};

/**
 * Exported Ports List (Base 2.4, 5.2.18.1.5, Figure 392, page 416, PDF page 444)
 *
 * Followed, from byte 8, by as many Exported Port Descriptor as the data says, each a 'struct xnvme_spec_base_exported_port_descriptor'
 */
struct __attribute__((packed)) xnvme_spec_base_exported_ports_list {
	uint16_t n; ///< Number of Exported Ports, bytes 1:0
	uint8_t rsvd2[6]; ///< Reserved, bytes 7:2
};

/**
 * Host Entry (Base 2.4, 5.2.19.1.3, Figure 401, page 421, PDF page 449)
 */
struct __attribute__((packed)) xnvme_spec_base_host_entry {
	uint64_t rsvd0; ///< Reserved, bytes 7:0
	uint8_t hostid[16]; ///< Host Identifier, bytes 23:8
	uint8_t hostnqn[256]; ///< Host NVMe Qualified Name, bytes 279:24
	uint8_t rsvd280[40]; ///< Reserved, bytes 319:280
};

/**
 * Subsystem Management (Base 2.4, 5.2.19.1.3, Figure 400, page 421, PDF page 449)
 *
 * Followed, from byte 256, by as many Host Entry as the data says, each a 'struct xnvme_spec_base_host_entry'
 *
 * Not members:
 * Exported NVM Subsystem Entry 1: at an offset the data gives, in 'host_entry'
 * Exported NVM Subsystem Entry N: at an offset the data gives, in 'host_entry'
 */
struct __attribute__((packed)) xnvme_spec_base_subsystem_management {
	uint8_t rsvd0[64]; ///< Reserved, bytes 63:0
	uint16_t numhent; ///< Number of Host Entries, bytes 65:64
	uint16_t numense; ///< Number of Exported NVM Subsystem Entries, bytes 67:66
	uint8_t rsvd68[188]; ///< Reserved, bytes 255:68
};

/**
 * Exported NVM Subsystem Entry (Base 2.4, 5.2.19.1.3, Figure 402, page 421, PDF page 449)
 */
struct __attribute__((packed)) xnvme_spec_base_exported_nvm_subsystem_entry {
	uint8_t rsvd0[24]; ///< Reserved, bytes 23:0
	uint8_t subnqn[256]; ///< NVM Subsystem NVMe Qualified Name, bytes 279:24
	uint16_t pidup; ///< Port ID of the Underlying Port, bytes 281:280
	uint8_t rsvd282[38]; ///< Reserved, bytes 319:282
};

/**
 * Controller State (Base 2.4, 5.2.21.2.3, Figure 431, page 435, PDF page 463)
 *
 * Followed, from byte 48, by as many NVMe Controller State as the data says
 *
 * Not members:
 * Vendor Specific Data: at an offset the data gives, in 'nvme_controller_state'
 */
struct __attribute__((packed)) xnvme_spec_base_controller_state {
	uint16_t ver; ///< Version, bytes 1:0
	uint8_t csattr; ///< Controller State Attributes, byte 2, see XNVME_SPEC_BASE_CONTROLLER_STATE_CSATTR_*
	uint8_t rsvd3[13]; ///< Reserved, bytes 15:3
	uint8_t nvmecss[16]; ///< NVMe Controller State Size, bytes 31:16
	uint8_t vss[16]; ///< Vendor Specific Size, bytes 47:32
};

/** Controller Suspended (Base 2.4, 5.2.21.2.3, Figure 431, page 435, PDF page 463); bit 0 of 'csattr' */
#define XNVME_SPEC_BASE_CONTROLLER_STATE_CSATTR_CP 0x1

/**
 * I/O Submission Queue State (Base 2.4, 5.2.21.2.3, Figure 433, page 438, PDF page 466)
 */
struct __attribute__((packed)) xnvme_spec_base_io_submission_queue_state {
	uint64_t iosqprp1; ///< I/O Submission PRP Entry 1, bytes 7:0, see XNVME_SPEC_BASE_IO_SUBMISSION_QUEUE_STATE_IOSQPRP1_*
	uint16_t iosqqsize; ///< I/O Submission Queue Size, bytes 9:8
	uint16_t iosqqid; ///< I/O Submission Queue Identifier, bytes 11:10
	uint16_t iosqcqid; ///< I/O Completion Queue Identifier, bytes 13:12
	uint16_t iosqa; ///< I/O Submission Queue Attributes, bytes 15:14, see XNVME_SPEC_BASE_IO_SUBMISSION_QUEUE_STATE_IOSQA_*
	uint16_t iosqhp; ///< I/O Submission Queue Head Pointer, bytes 17:16
	uint16_t iosqtp; ///< I/O Submission Queue Tail Pointer, bytes 19:18
	uint32_t rsvd20; ///< Reserved, bytes 23:20
};

/** PRP Entry 1 (Base 2.4, 5.3.2, Figure 575, page 526, PDF page 555); bits 63:0 of 'iosqprp1' */
#define XNVME_SPEC_BASE_IO_SUBMISSION_QUEUE_STATE_IOSQPRP1_PRP1 0xffffffffffffffff
/** I/O Submission Queue Priority (Base 2.4, 5.2.21.2.3, Figure 433, page 438, PDF page 466); bits 2:1 of 'iosqa' */
#define XNVME_SPEC_BASE_IO_SUBMISSION_QUEUE_STATE_IOSQA_IOSQQPRIO 0x6
/** I/O Submission Queue Physically Contiguous (Base 2.4, 5.2.21.2.3, Figure 433, page 438, PDF page 466); bit 0 of 'iosqa' */
#define XNVME_SPEC_BASE_IO_SUBMISSION_QUEUE_STATE_IOSQA_IOSQPC 0x1

/**
 * NVMe Controller State (Base 2.4, 5.2.21.2.3, Figure 432, page 435, PDF page 463)
 *
 * Followed, from byte 60, by as many I/O Submission Queue State as the data says, each a 'struct xnvme_spec_base_io_submission_queue_state'
 *
 * Not members:
 * I/O Completion Queue State 0: at an offset the data gives, in 'io_submission_queue_state'
 * I/O Completion Queue State 1: at an offset the data gives, in 'io_submission_queue_state'
 * I/O Completion Queue State NIOSQ-1: at an offset the data gives, in 'io_submission_queue_state'
 */
struct __attribute__((packed)) xnvme_spec_base_nvme_controller_state {
	uint16_t ver; ///< Version, bytes 1:0
	uint16_t number_of_outstanding_asynchronous_event_request_commands; ///< Number of Outstanding Asynchronous Event Request Commands, bytes 3:2
	uint16_t nper; ///< Number of Pending Events to Report, bytes 5:4
	uint8_t rsvd6; ///< Reserved, byte 6
	uint8_t ama; ///< Admin Migration Attributes, byte 7, see XNVME_SPEC_BASE_NVME_CONTROLLER_STATE_AMA_*
	uint32_t aqa; ///< Admin Queue Attributes, bytes 11:8
	uint32_t rsvd12; ///< Reserved, bytes 15:12
	uint64_t asq; ///< Admin Submission Queue Base Address, bytes 23:16
	uint16_t asqhp; ///< Admin Submission Queue Header Pointer, bytes 25:24
	uint16_t asqtp; ///< Admin Submission Queue Tail Pointer, bytes 27:26
	uint32_t rsvd28; ///< Reserved, bytes 31:28
	uint64_t acq; ///< Admin Completion Queue Base Address, bytes 39:32
	uint16_t acqhp; ///< Admin Completion Queue Head Pointer, bytes 41:40
	uint16_t acqtp; ///< Admin Completion Queue Tail Pointer, bytes 43:42
	uint32_t intm; ///< Interrupt Mask, bytes 47:44
	uint32_t cc; ///< Controller Configuration, bytes 51:48
	uint32_t csts; ///< Controller Status, bytes 55:52
	uint16_t niosq; ///< Number of I/O Submission Queues, bytes 57:56
	uint16_t niocq; ///< Number of I/O Completion Queues, bytes 59:58
};

/** Slot 0 Phase Tag (Base 2.4, 5.2.21.2.3, Figure 432, page 435, PDF page 463); bit 0 of 'ama' */
#define XNVME_SPEC_BASE_NVME_CONTROLLER_STATE_AMA_S0PT 0x1

/**
 * I/O Completion Queue State (Base 2.4, 5.2.21.2.3, Figure 434, page 438, PDF page 466)
 */
struct __attribute__((packed)) xnvme_spec_base_io_completion_queue_state {
	uint64_t iocqprp1; ///< I/O Completion Queue Size PRP Entry 1, bytes 7:0, see XNVME_SPEC_BASE_IO_COMPLETION_QUEUE_STATE_IOCQPRP1_*
	uint16_t iocqqsize; ///< I/O Completion Queue Size, bytes 9:8
	uint16_t iocqqid; ///< I/O Completion Queue Identifier, bytes 11:10
	uint16_t iocqhp; ///< I/O Completion Queue Head Pointer, bytes 13:12
	uint16_t iocqtp; ///< I/O Completion Queue Tail Pointer, bytes 15:14
	uint32_t iocqa; ///< I/O Completion Queue Attributes, bytes 19:16, see XNVME_SPEC_BASE_IO_COMPLETION_QUEUE_STATE_IOCQA_*
	uint32_t rsvd20; ///< Reserved, bytes 23:20
};

/** PRP Entry 1 (Base 2.4, 5.3.1, Figure 571, page 524, PDF page 553); bits 63:0 of 'iocqprp1' */
#define XNVME_SPEC_BASE_IO_COMPLETION_QUEUE_STATE_IOCQPRP1_PRP1 0xffffffffffffffff
/** I/O Completion Queue Interrupt Vector (Base 2.4, 5.2.21.2.3, Figure 434, page 438, PDF page 466); bits 31:16 of 'iocqa' */
#define XNVME_SPEC_BASE_IO_COMPLETION_QUEUE_STATE_IOCQA_IOCQIV 0xffff0000
/** I/O Completion Queue Interrupt Vector Coalescing Disabled (Base 2.4, 5.2.21.2.3, Figure 434, page 438, PDF page 466); bit 3 of 'iocqa' */
#define XNVME_SPEC_BASE_IO_COMPLETION_QUEUE_STATE_IOCQA_IOCQIVCD 0x8
/** Slot 0 Phase Tag (Base 2.4, 5.2.21.2.3, Figure 434, page 438, PDF page 466); bit 2 of 'iocqa' */
#define XNVME_SPEC_BASE_IO_COMPLETION_QUEUE_STATE_IOCQA_S0PT 0x4
/** I/O Completion Queue Interrupts Enabled (Base 2.4, 5.2.21.2.3, Figure 434, page 438, PDF page 466); bit 1 of 'iocqa' */
#define XNVME_SPEC_BASE_IO_COMPLETION_QUEUE_STATE_IOCQA_IOCQIEN 0x2
/** I/O Completion Queue Physically Contiguous (Base 2.4, 5.2.21.2.3, Figure 434, page 438, PDF page 466); bit 0 of 'iocqa' */
#define XNVME_SPEC_BASE_IO_COMPLETION_QUEUE_STATE_IOCQA_IOCQPC 0x1

/**
 * Pending Event (Base 2.4, 5.2.21.2.3, Figure 435, page 439, PDF page 467)
 */
struct __attribute__((packed)) xnvme_spec_base_pending_event {
	uint32_t cqed[2]; ///< Completion Queue Entry Dword, 2 of 4 bytes, bytes 7:0
};

/**
 * Namespace Management (Base 2.4, 5.2.25, Figure 448, page 445, PDF page 473)
 */
struct __attribute__((packed)) xnvme_spec_base_namespace_management {
	uint8_t siocs[512]; ///< Specific to the I/O Command Set, bytes 511:0
	uint8_t rsvd512[512]; ///< Reserved, bytes 1023:512
	uint8_t vs[3072]; ///< Vendor Specific, bytes 4095:1024
};

/**
 * Autonomous Power State Transition (Base 2.4, 5.2.30.1.7, Figure 476, page 467, PDF page 495)
 */
struct __attribute__((packed)) xnvme_spec_base_autonomous_power_state_transition {
	uint64_t ps[32]; ///< Power State, 32 of 8 bytes of XNVME_SPEC_BASE_AUTONOMOUS_POWER_STATE_TRANSITION_FIGURE477_*, bytes 255:0
};

/** Idle Time Prior to Transition (Base 2.4, 5.2.30.1.7, Figure 477, page 467, PDF page 495); bits 31:8 of 'Autonomous Power State Transition' */
#define XNVME_SPEC_BASE_AUTONOMOUS_POWER_STATE_TRANSITION_FIGURE477_ITPT 0xffffff00
/** Idle Transition Power State (Base 2.4, 5.2.30.1.7, Figure 477, page 467, PDF page 495); bits 7:3 of 'Autonomous Power State Transition' */
#define XNVME_SPEC_BASE_AUTONOMOUS_POWER_STATE_TRANSITION_FIGURE477_ITPS 0xf8

/**
 * Timestamp (Base 2.4, 5.2.30.1.8, Figure 479, page 468, PDF page 496)
 */
struct __attribute__((packed)) xnvme_spec_base_timestamp {
	uint8_t tstmp[6]; ///< Timestamp, bytes 5:0
	uint16_t rsvd6; ///< Reserved, bytes 7:6
};

/**
 * Timestamp (Base 2.4, 5.2.30.1.8, Figure 480, page 468, PDF page 496)
 */
struct __attribute__((packed)) xnvme_spec_base_timestamp_figure480 {
	uint8_t tstmp[6]; ///< Timestamp, bytes 5:0
	uint8_t tstmps; ///< Timestamp Attribute, byte 6, see XNVME_SPEC_BASE_TIMESTAMP_FIGURE480_TSTMPS_*
	uint8_t rsvd7; ///< Reserved, byte 7
};

/** Timestamp Origin (Base 2.4, 5.2.30.1.8, Figure 480, page 468, PDF page 496); bits 3:1 of 'tstmps' */
#define XNVME_SPEC_BASE_TIMESTAMP_FIGURE480_TSTMPS_TSTMPO 0xe
/** Synch (Base 2.4, 5.2.30.1.8, Figure 480, page 468, PDF page 496); bit 0 of 'tstmps' */
#define XNVME_SPEC_BASE_TIMESTAMP_FIGURE480_TSTMPS_SYNC 0x1

/**
 * Predictable Latency Mode - Deterministic Threshold Configuration (Base 2.4, 5.2.30.1.13, Figure 488, page 472, PDF page 500)
 */
struct __attribute__((packed)) xnvme_spec_base_deterministic_threshold_configuration {
	uint16_t enev; ///< Enable Event, bytes 1:0, see XNVME_SPEC_BASE_DETERMINISTIC_THRESHOLD_CONFIGURATION_ENEV_*
	uint8_t rsvd2[30]; ///< Reserved, bytes 31:2
	uint64_t dtrt; ///< DTWIN Reads Threshold, bytes 39:32
	uint64_t dtwt; ///< DTWIN Writes Threshold, bytes 47:40
	uint64_t dttt; ///< DTWIN Time Threshold, bytes 55:48
	uint8_t rsvd56[456]; ///< Reserved, bytes 511:56
};

/** Deterministic Excursion (Base 2.4, 5.2.30.1.13, Figure 488, page 472, PDF page 500); bit 15 of 'enev' */
#define XNVME_SPEC_BASE_DETERMINISTIC_THRESHOLD_CONFIGURATION_ENEV_DTE 0x8000
/** Exceeded Value (Base 2.4, 5.2.30.1.13, Figure 488, page 472, PDF page 500); bit 14 of 'enev' */
#define XNVME_SPEC_BASE_DETERMINISTIC_THRESHOLD_CONFIGURATION_ENEV_EXV 0x4000
/** DTWIN Time Warning (Base 2.4, 5.2.30.1.13, Figure 488, page 472, PDF page 500); bit 2 of 'enev' */
#define XNVME_SPEC_BASE_DETERMINISTIC_THRESHOLD_CONFIGURATION_ENEV_DTTW 0x4
/** DTWIN Writes Warning (Base 2.4, 5.2.30.1.13, Figure 488, page 472, PDF page 500); bit 1 of 'enev' */
#define XNVME_SPEC_BASE_DETERMINISTIC_THRESHOLD_CONFIGURATION_ENEV_DTWW 0x2
/** DTWIN Reads Warning (Base 2.4, 5.2.30.1.13, Figure 488, page 472, PDF page 500); bit 0 of 'enev' */
#define XNVME_SPEC_BASE_DETERMINISTIC_THRESHOLD_CONFIGURATION_ENEV_DTRW 0x1

/**
 * Host Behavior Support (Base 2.4, 5.2.30.1.15, Figure 491, page 474, PDF page 502)
 */
struct __attribute__((packed)) xnvme_spec_base_host_behavior_support {
	uint8_t acre; ///< Advanced Command Retry Enable, byte 0
	uint8_t etdas; ///< Extended Telemetry Data Area 4 Supported, byte 1
	uint8_t lbafee; ///< LBA Format Extension Enable, byte 2
	uint8_t hdisns; ///< Host Dispersed Namespace Support, byte 3
	uint16_t cdfe; ///< Copy Descriptor Formats Enable, bytes 5:4, see XNVME_SPEC_BASE_HOST_BEHAVIOR_SUPPORT_CDFE_*
	uint8_t rsvd6[506]; ///< Reserved, bytes 511:6
};

/** Copy Descriptor Format 4h Enable (Base 2.4, 5.2.30.1.15, Figure 491, page 474, PDF page 502); bit 4 of 'cdfe' */
#define XNVME_SPEC_BASE_HOST_BEHAVIOR_SUPPORT_CDFE_CDF4E 0x10
/** Copy Descriptor Format 3h Enable (Base 2.4, 5.2.30.1.15, Figure 491, page 474, PDF page 502); bit 3 of 'cdfe' */
#define XNVME_SPEC_BASE_HOST_BEHAVIOR_SUPPORT_CDFE_CDF3E 0x8
/** Copy Descriptor Format 2h Enable (Base 2.4, 5.2.30.1.15, Figure 491, page 474, PDF page 502); bit 2 of 'cdfe' */
#define XNVME_SPEC_BASE_HOST_BEHAVIOR_SUPPORT_CDFE_CDF2E 0x4
/** Not Used (Base 2.4, 5.2.30.1.15, Figure 491, page 474, PDF page 502); bits 1:0 of 'cdfe' */
#define XNVME_SPEC_BASE_HOST_BEHAVIOR_SUPPORT_CDFE_NUSED 0x3

/**
 * Supported FDP Event Descriptor (Base 2.4, 5.2.30.1.22, Figure 506, page 480, PDF page 508)
 */
struct __attribute__((packed)) xnvme_spec_base_supported_fdp_event_descriptor {
	uint8_t fdpet; ///< FDP Event Type, byte 0
	uint8_t fdpeta; ///< FDP Event Type Attributes, byte 1, see XNVME_SPEC_BASE_SUPPORTED_FDP_EVENT_DESCRIPTOR_FDPETA_*
};

/** FDP Event Enable (Base 2.4, 5.2.30.1.22, Figure 506, page 480, PDF page 508); bit 0 of 'fdpeta' */
#define XNVME_SPEC_BASE_SUPPORTED_FDP_EVENT_DESCRIPTOR_FDPETA_FDPEE 0x1

/**
 * Namespace Admin Label (Base 2.4, 5.2.30.1.23, Figure 507, page 481, PDF page 509)
 */
struct __attribute__((packed)) xnvme_spec_base_namespace_admin_label {
	uint8_t nsal[256]; ///< Namespace Admin Label, bytes 255:0
};

/**
 * Controller Data Queue (Base 2.4, 5.2.30.1.24, Figure 511, page 483, PDF page 511)
 */
struct __attribute__((packed)) xnvme_spec_base_controller_data_queue {
	uint32_t hp; ///< Head Pointer, bytes 3:0
	uint32_t tpt; ///< Tail Pointer Trigger, bytes 7:4
	uint8_t rsvd8[504]; ///< Reserved, bytes 511:8
};

/**
 * Security Personality (Base 2.4, 5.2.30.1.25.3, Figure 516, page 488, PDF page 517)
 */
struct __attribute__((packed)) xnvme_spec_base_security_personality {
	uint32_t sps; ///< Security Protocol Status, bytes 3:0, see XNVME_SPEC_BASE_SECURITY_PERSONALITY_SPS_*
	uint32_t ssp; ///< Supported Security Protocol, bytes 7:4, see XNVME_SPEC_BASE_SECURITY_PERSONALITY_SSP_*
};

/** Vendor Specific Protocol FF (Base 2.4, 5.2.30.1.25.3, Figure 516, page 488, PDF page 517); bit 31 of 'sps' */
#define XNVME_SPEC_BASE_SECURITY_PERSONALITY_SPS_VSPFF 0x80000000
/** Vendor Specific Protocol F1 (Base 2.4, 5.2.30.1.25.3, Figure 516, page 488, PDF page 517); bit 17 of 'sps' */
#define XNVME_SPEC_BASE_SECURITY_PERSONALITY_SPS_VSPF1 0x20000
/** Vendor Specific Protocol F0 (Base 2.4, 5.2.30.1.25.3, Figure 516, page 488, PDF page 517); bit 16 of 'sps' */
#define XNVME_SPEC_BASE_SECURITY_PERSONALITY_SPS_VSPF0 0x10000
/** Authentication in Host Attachments of Transient Storage Devices Protocol Status (Base 2.4, 5.2.30.1.25.3, Figure 516, page 488, PDF page 517); bit 2 of 'sps' */
#define XNVME_SPEC_BASE_SECURITY_PERSONALITY_SPS_AHATSDPS 0x4
/** TCG Security Protocol Status (Base 2.4, 5.2.30.1.25.3, Figure 516, page 488, PDF page 517); bit 1 of 'sps' */
#define XNVME_SPEC_BASE_SECURITY_PERSONALITY_SPS_TCGPST 0x2
/** Vendor Specific Protocol FF (Base 2.4, 5.2.30.1.25.3, Figure 516, page 488, PDF page 517); bit 31 of 'ssp' */
#define XNVME_SPEC_BASE_SECURITY_PERSONALITY_SSP_VSP15 0x80000000
/** Vendor Specific Protocol F1 (Base 2.4, 5.2.30.1.25.3, Figure 516, page 488, PDF page 517); bit 17 of 'ssp' */
#define XNVME_SPEC_BASE_SECURITY_PERSONALITY_SSP_VSP1 0x20000
/** Vendor Specific Protocol F0 (Base 2.4, 5.2.30.1.25.3, Figure 516, page 488, PDF page 517); bit 16 of 'ssp' */
#define XNVME_SPEC_BASE_SECURITY_PERSONALITY_SSP_VSP0 0x10000
/** Authentication in Host Attachments of Transient Storage Devices Protocol Support (Base 2.4, 5.2.30.1.25.3, Figure 516, page 488, PDF page 517); bit 2 of 'ssp' */
#define XNVME_SPEC_BASE_SECURITY_PERSONALITY_SSP_AHATSDPS 0x4
/** TCG Security Protocol Support (Base 2.4, 5.2.30.1.25.3, Figure 516, page 488, PDF page 517); bit 1 of 'ssp' */
#define XNVME_SPEC_BASE_SECURITY_PERSONALITY_SSP_TCGPSUP 0x2

/**
 * Start Voltage Measurements (Base 2.4, 5.2.30.1.30, Figure 527, page 499, PDF page 528)
 */
struct __attribute__((packed)) xnvme_spec_base_start_voltage_measurements {
	uint16_t lvolta; ///< Log Voltage Threshold Attributes, bytes 1:0, see XNVME_SPEC_BASE_START_VOLTAGE_MEASUREMENTS_LVOLTA_*
	uint16_t svmt; ///< Stop Voltage Measurement Time, bytes 3:2
	uint32_t lvolt; ///< Log Voltage Threshold, bytes 7:4, see XNVME_SPEC_BASE_START_VOLTAGE_MEASUREMENTS_LVOLT_*
	uint8_t rsvd8[4088]; ///< Reserved, bytes 4095:8
};

/** Voltage Measurement Log Threshold (Base 2.4, 5.2.30.1.30, Figure 527, page 499, PDF page 528); bit 2 of 'lvolta' */
#define XNVME_SPEC_BASE_START_VOLTAGE_MEASUREMENTS_LVOLTA_VMLT 0x4
/** Voltage Sensor Selected (Base 2.4, 5.2.30.1.30, Figure 527, page 499, PDF page 528); bits 1:0 of 'lvolta' */
#define XNVME_SPEC_BASE_START_VOLTAGE_MEASUREMENTS_LVOLTA_VSSEL 0x3
/** Log Overvoltage Threshold (Base 2.4, 5.2.30.1.30, Figure 527, page 499, PDF page 528); bits 29:16 of 'lvolt' */
#define XNVME_SPEC_BASE_START_VOLTAGE_MEASUREMENTS_LVOLT_LOVT 0x3fff0000
/** Log Undervoltage Threshold (Base 2.4, 5.2.30.1.30, Figure 527, page 499, PDF page 528); bits 13:0 of 'lvolt' */
#define XNVME_SPEC_BASE_START_VOLTAGE_MEASUREMENTS_LVOLT_LUVT 0x3fff

/**
 * Host Management Agent Address (Base 2.4, 5.2.30.1.32, Figure 529, page 500, PDF page 529)
 */
struct __attribute__((packed)) xnvme_spec_base_host_management_agent_address {
	uint32_t rsvd0; ///< Reserved, bytes 3:0
	uint8_t hmaa[508]; ///< Host Management Agent Address, bytes 511:4
};

/**
 * Host Metadata (Base 2.4, 5.2.30.1.33, Figure 532, page 503, PDF page 532)
 *
 * Followed, from byte 2, by as many Metadata Element Descriptor as the data says
 *
 * Not members:
 * Metadata Element Descriptor 1: at an offset the data gives, in 'metadata_element_descriptor'
 * Metadata Element Descriptor N: at an offset the data gives, in 'metadata_element_descriptor'
 */
struct __attribute__((packed)) xnvme_spec_base_host_metadata {
	uint8_t nmed; ///< Number of Metadata Element Descriptors, byte 0
	uint8_t rsvd1; ///< Reserved, byte 1
};

/**
 * Host Identifier (Base 2.4, 5.2.30.1.35, Figure 538, page 507, PDF page 536)
 */
struct __attribute__((packed)) xnvme_spec_base_host_identifier {
	uint8_t hostid[16]; ///< Host Identifier, bytes 15:0
};

/**
 * Host Memory Buffer - Attributes (Base 2.4, 5.2.30.2.3, Figure 553, page 515, PDF page 545)
 */
struct __attribute__((packed)) xnvme_spec_base_attributes {
	uint32_t hsize; ///< Host Memory Buffer Size, bytes 3:0
	uint32_t hmdlal; ///< Host Memory Descriptor List Address Lower, bytes 7:4
	uint32_t hmdlau; ///< Host Memory Descriptor List Address Upper, bytes 11:8
	uint32_t hmdlec; ///< Host Memory Descriptor List Entry Count, bytes 15:12
	uint8_t rsvd16[4080]; ///< Reserved, bytes 4095:16
};

/**
 * Tracked Memory Changed Descriptor (Base 2.4, 5.2.31.1.1, Figure 559, page 519, PDF page 548)
 */
struct __attribute__((packed)) xnvme_spec_base_tracked_memory_changed_descriptor {
	uint64_t saddr; ///< Start Address, bytes 7:0
	uint32_t len; ///< Length, bytes 11:8
	uint32_t rsvd12; ///< Reserved, bytes 15:12
};

/**
 * Tracked Memory Change (Base 2.4, 5.2.31.1.1, Figure 558, page 518, PDF page 547)
 *
 * Followed, from byte 16, by as many Tracked Memory Changed Descriptor as the data says, each a 'struct xnvme_spec_base_tracked_memory_changed_descriptor'
 */
struct __attribute__((packed)) xnvme_spec_base_tracked_memory_change {
	uint8_t ver; ///< Version, byte 0
	uint8_t attrb; ///< Attributes, byte 1, see XNVME_SPEC_BASE_TRACKED_MEMORY_CHANGE_ATTRB_*
	uint16_t cntlid; ///< Controller Identifier, bytes 3:2
	uint32_t ntmcd; ///< Number of Tracked Memory Changed Descriptors, bytes 7:4
	uint16_t rpmpg; ///< Reported Memory Range Granularity, bytes 9:8
	uint8_t rsvd10[6]; ///< Reserved, bytes 15:10
};

/** Suspended (Base 2.4, 5.2.31.1.1, Figure 558, page 518, PDF page 547); bit 1 of 'attrb' */
#define XNVME_SPEC_BASE_TRACKED_MEMORY_CHANGE_ATTRB_SUSP 0x2
/** More To Report (Base 2.4, 5.2.31.1.1, Figure 558, page 518, PDF page 547); bit 0 of 'attrb' */
#define XNVME_SPEC_BASE_TRACKED_MEMORY_CHANGE_ATTRB_MTR 0x1

/**
 * Memory Range Tracking Descriptor (Base 2.4, 5.2.32.1.2, Figure 567, page 522, PDF page 552)
 */
struct __attribute__((packed)) xnvme_spec_base_memory_range_tracking_descriptor {
	uint64_t saddr; ///< Address, bytes 7:0
	uint32_t len; ///< Length, bytes 11:8
};

/**
 * Track Memory Changes (Base 2.4, 5.2.32.1.2, Figure 566, page 522, PDF page 551)
 *
 * Followed, from byte 8, by as many Memory Range Tracking Descriptor as the data says, each a 'struct xnvme_spec_base_memory_range_tracking_descriptor'
 */
struct __attribute__((packed)) xnvme_spec_base_track_memory_changes {
	uint8_t ver; ///< Version, byte 0
	uint16_t rsvd1; ///< Reserved, bytes 2:1
	uint8_t rmrtg; ///< Requested Memory Range Tracking Granularity, byte 3
	uint32_t rnmrtd; ///< Number of Memory Range Tracking Descriptors, bytes 7:4
};

/**
 * Discovery Information Management - Data (Base 2.4, 5.4.2, Figure 598, page 536, PDF page 565)
 *
 * Followed, from byte 1024, by as many Discovery Information Entry as the data says
 *
 * Not members:
 * Discovery Information Entry NUMENT-1: at an offset the data gives, in 'discovery_information_entry'
 */
struct __attribute__((packed)) xnvme_spec_base_data {
	uint32_t tdl; ///< Total Data Length, bytes 3:0
	uint32_t rsvd4; ///< Reserved, bytes 7:4
	uint64_t nument; ///< Number of Entries, bytes 15:8
	uint16_t entfmt; ///< Entry Format, bytes 17:16
	uint16_t etype; ///< Entity Type, bytes 19:18
	uint8_t portlcl; ///< Port Local, byte 20
	uint8_t rsvd21; ///< Reserved, byte 21
	uint16_t ektype; ///< Entry Key Type, bytes 23:22, see XNVME_SPEC_BASE_DATA_EKTYPE_*
	uint8_t eid[256]; ///< Entity Identifier, bytes 279:24
	uint8_t ename[256]; ///< Entity Name, bytes 535:280
	uint8_t ever[64]; ///< Entity Version, bytes 599:536
	uint8_t rsvd600[424]; ///< Reserved, bytes 1023:600
};

/** Transport Address (Base 2.4, 5.4.2, Figure 598, page 536, PDF page 565); bit 6 of 'ektype' */
#define XNVME_SPEC_BASE_DATA_EKTYPE_TRADDR 0x40
/** Port ID (Base 2.4, 5.4.2, Figure 598, page 536, PDF page 565); bit 5 of 'ektype' */
#define XNVME_SPEC_BASE_DATA_EKTYPE_PORTID 0x20
/** Transport Type (Base 2.4, 5.4.2, Figure 598, page 536, PDF page 565); bit 4 of 'ektype' */
#define XNVME_SPEC_BASE_DATA_EKTYPE_TRTYPE 0x10
/** Address Family (Base 2.4, 5.4.2, Figure 598, page 536, PDF page 565); bit 3 of 'ektype' */
#define XNVME_SPEC_BASE_DATA_EKTYPE_ADRFAM 0x8
/** Transport Service Identifier (Base 2.4, 5.4.2, Figure 598, page 536, PDF page 565); bit 2 of 'ektype' */
#define XNVME_SPEC_BASE_DATA_EKTYPE_TRSVCID 0x4
/** Transport Specific Address Subtype (Base 2.4, 5.4.2, Figure 598, page 536, PDF page 565); bit 1 of 'ektype' */
#define XNVME_SPEC_BASE_DATA_EKTYPE_TSAS 0x2
/** NVMe Qualified Name (Base 2.4, 5.4.2, Figure 598, page 536, PDF page 565); bit 0 of 'ektype' */
#define XNVME_SPEC_BASE_DATA_EKTYPE_NQN 0x1

/**
 * The values of Entity Type (Base 2.4, 5.4.2, Figure 598, page 536, PDF page 565)
 */
enum xnvme_spec_base_data_etype {
	XNVME_SPEC_BASE_DATA_ETYPE_HOST = 0x1, ///< Host (Base 2.4, 5.4.2, Figure 598, page 536, PDF page 565)
};

/**
 * Extended Discovery Information Entry (Base 2.4, 5.4.2, Figure 599, page 538, PDF page 567)
 *
 * Followed, from byte 1032, by as many Extended Attribute as the data says
 *
 * Not members:
 * Extended Attribute N: at an offset the data gives, in 'extended_attribute'
 */
struct __attribute__((packed)) xnvme_spec_base_extended_discovery_information_entry {
	uint8_t trtype; ///< Transport Type, byte 0
	uint8_t adrfam; ///< Address Family, byte 1
	uint8_t subtype; ///< Subsystem Type, byte 2
	uint8_t treq; ///< Transport Requirements, byte 3
	uint16_t portid; ///< Port ID, bytes 5:4
	uint16_t cntlid; ///< Controller ID, bytes 7:6
	uint16_t asqsz; ///< Admin Max SQ Size, bytes 9:8
	uint8_t rsvd10[22]; ///< Reserved, bytes 31:10
	uint8_t trsvcid[32]; ///< Transport Service Identifier, bytes 63:32
	uint8_t rsvd64[192]; ///< Reserved, bytes 255:64
	uint8_t nqn[256]; ///< NVMe Qualified Name, bytes 511:256
	uint8_t traddr[256]; ///< Transport Address, bytes 767:512
	uint8_t tsas[256]; ///< Transport Specific Address Subtype, bytes 1023:768
	uint32_t tel; ///< Total Entry Length, bytes 1027:1024
	uint16_t numexat; ///< Number of Extended Attributes, bytes 1029:1028
	uint16_t rsvd1030; ///< Reserved, bytes 1031:1030
};

/**
 * Extended Attribute (Base 2.4, 5.4.2, Figure 600, page 540, PDF page 569)
 *
 * Followed, from byte 4, by as many Extended Attribute Value as the data says
 */
struct __attribute__((packed)) xnvme_spec_base_extended_attribute {
	uint16_t exattype; ///< Extended Attribute Type, bytes 1:0
	uint16_t exatlen; ///< Extended Attribute Length, bytes 3:2
};

/**
 * The values of Extended Attribute Type (Base 2.4, 5.4.2, Figure 600, page 540, PDF page 569)
 */
enum xnvme_spec_base_extended_attribute_exattype {
	XNVME_SPEC_BASE_EXTENDED_ATTRIBUTE_EXATTYPE_HOST_IDENTIFIER = 0x1, ///< Host Identifier (Base 2.4, 5.4.2, Figure 600, page 540, PDF page 569)
};

/**
 * Create (Base 2.4, 5.4.6.1.1, Figure 620, page 545, PDF page 575)
 */
struct __attribute__((packed)) xnvme_spec_base_create {
	uint8_t subnqn[256]; ///< Exported NVM Subsystem NVMe Qualified Name, bytes 255:0
	uint16_t epid; ///< Exported Port ID, bytes 257:256
	uint16_t pidud; ///< Port ID of the Underlying Port, bytes 259:258
	uint8_t trsvcid[32]; ///< Transport Service ID, bytes 291:260
	uint8_t rsvd292[28]; ///< Reserved, bytes 319:292
};

/**
 * Create Completion Queue Entry Dword 0 (Base 2.4, 5.4.6.1.1, Figure 621, page 546, PDF page 575)
 */
struct __attribute__((packed)) xnvme_spec_base_create_completion_queue_entry_dword_0 {
	uint16_t epid; ///< Exported Port ID, bytes 1:0
	uint16_t rsvd2; ///< Reserved, bytes 3:2
};

/**
 * Delete (Base 2.4, 5.4.6.1.2, Figure 622, page 546, PDF page 576)
 */
struct __attribute__((packed)) xnvme_spec_base_delete {
	uint8_t subnqn[256]; ///< NVM Subsystem NVMe Qualified Name, bytes 255:0
	uint16_t epid; ///< Exported Port ID, bytes 257:256
	uint8_t rsvd258[62]; ///< Reserved, bytes 319:258
};

/**
 * Authentication Receive Command - Submission Queue Entry (Base 2.4, 6.1, Figure 631, page 549, PDF page 579)
 */
struct __attribute__((packed)) xnvme_spec_base_submission_queue_entry {
	uint32_t cdw0; ///< Command Dword 0, bytes 3:0
	uint8_t fctype; ///< Fabrics Command Type, byte 4
	uint8_t rsvd5[19]; ///< Reserved, bytes 23:5
	uint8_t sgl1[16]; ///< SGL Descriptor 1, bytes 39:24
	uint8_t rsvd40; ///< Reserved, byte 40
	uint8_t spsp[2]; ///< SP Specific, 2 of 1 bytes, bytes 42:41
	uint8_t secp; ///< Security Protocol, byte 43
	uint32_t al; ///< Allocation Length, bytes 47:44
	uint8_t rsvd48[16]; ///< Reserved, bytes 63:48
};

/**
 * Authentication Receive Response (Base 2.4, 6.1, Figure 632, page 550, PDF page 580)
 */
struct __attribute__((packed)) xnvme_spec_base_authentication_receive_response {
	uint64_t rsvd0; ///< Reserved, bytes 7:0
	uint16_t sqhd; ///< SQ Head Pointer, bytes 9:8
	uint16_t rsvd10; ///< Reserved, bytes 11:10
	uint16_t cid; ///< Command Identifier, bytes 13:12
	uint16_t sts; ///< Status Info, bytes 15:14, see XNVME_SPEC_BASE_AUTHENTICATION_RECEIVE_RESPONSE_STS_*
};

/** Status (Base 2.4, 6.1, Figure 632, page 550, PDF page 580); bits 15:1 of 'sts' */
#define XNVME_SPEC_BASE_AUTHENTICATION_RECEIVE_RESPONSE_STS_STATUS 0xfffe

/**
 * Authentication Send Command - Submission Queue Entry (Base 2.4, 6.2, Figure 633, page 550, PDF page 580)
 */
struct __attribute__((packed)) xnvme_spec_base_submission_queue_entry_figure633 {
	uint32_t cdw0; ///< Command Dword 0, bytes 3:0
	uint8_t fctype; ///< Fabrics Command Type, byte 4
	uint8_t rsvd5[19]; ///< Reserved, bytes 23:5
	uint8_t sgl1[16]; ///< SGL Descriptor 1, bytes 39:24
	uint8_t rsvd40; ///< Reserved, byte 40
	uint8_t spsp[2]; ///< SP Specific, 2 of 1 bytes, bytes 42:41
	uint8_t secp; ///< Security Protocol, byte 43
	uint32_t tl; ///< Transfer Length, bytes 47:44
	uint8_t rsvd48[16]; ///< Reserved, bytes 63:48
};

/**
 * Authentication Send Response (Base 2.4, 6.2, Figure 634, page 551, PDF page 581)
 */
struct __attribute__((packed)) xnvme_spec_base_authentication_send_response {
	uint64_t rsvd0; ///< Reserved, bytes 7:0
	uint16_t sqhd; ///< SQ Head Pointer, bytes 9:8
	uint16_t rsvd10; ///< Reserved, bytes 11:10
	uint16_t cid; ///< Command Identifier, bytes 13:12
	uint16_t sts; ///< Status Info, bytes 15:14, see XNVME_SPEC_BASE_AUTHENTICATION_SEND_RESPONSE_STS_*
};

/** Status (Base 2.4, 6.2, Figure 634, page 551, PDF page 581); bits 15:1 of 'sts' */
#define XNVME_SPEC_BASE_AUTHENTICATION_SEND_RESPONSE_STS_STATUS 0xfffe

/**
 * Connect Command - Submission Queue Entry (Base 2.4, 6.3, Figure 635, page 553, PDF page 583)
 */
struct __attribute__((packed)) xnvme_spec_base_submission_queue_entry_figure635 {
	uint32_t cdw0; ///< Command Dword 0, bytes 3:0
	uint8_t fctype; ///< Fabrics Command Type, byte 4
	uint8_t rsvd5[19]; ///< Reserved, bytes 23:5
	uint8_t sgl1[16]; ///< SGL Descriptor 1, bytes 39:24
	uint16_t recfmt; ///< Record Format, bytes 41:40
	uint16_t qid; ///< Queue ID, bytes 43:42
	uint16_t sqsize; ///< Submission Queue Size, bytes 45:44
	uint8_t cattr; ///< Connect Attributes, byte 46, see XNVME_SPEC_BASE_SUBMISSION_QUEUE_ENTRY_FIGURE635_CATTR_*
	uint8_t rsvd47; ///< Reserved, byte 47
	uint32_t kato; ///< Keep Alive Timeout, bytes 51:48
	uint16_t nvmsetid; ///< NVM Set Identifier, bytes 53:52
	uint8_t rsvd54[10]; ///< Reserved, bytes 63:54
};

/** Connecting Entity (Base 2.4, 6.3, Figure 635, page 553, PDF page 583); bit 4 of 'cattr' */
#define XNVME_SPEC_BASE_SUBMISSION_QUEUE_ENTRY_FIGURE635_CATTR_CONNENT 0x10
/** Individual I/O Queue Deletion Support (Base 2.4, 6.3, Figure 635, page 553, PDF page 583); bit 3 of 'cattr' */
#define XNVME_SPEC_BASE_SUBMISSION_QUEUE_ENTRY_FIGURE635_CATTR_INDIVIOQDELS 0x8
/** Disable SQ Flow Control (Base 2.4, 6.3, Figure 635, page 553, PDF page 583); bit 2 of 'cattr' */
#define XNVME_SPEC_BASE_SUBMISSION_QUEUE_ENTRY_FIGURE635_CATTR_DISSQFC 0x4
/** Priority Class (Base 2.4, 6.3, Figure 635, page 553, PDF page 583); bits 1:0 of 'cattr' */
#define XNVME_SPEC_BASE_SUBMISSION_QUEUE_ENTRY_FIGURE635_CATTR_PRIOCLASS 0x3

/**
 * The values of Priority Class (Base 2.4, 6.3, Figure 635, page 553, PDF page 583)
 */
enum xnvme_spec_base_submission_queue_entry_figure635_cattr_prioclass {
	XNVME_SPEC_BASE_SUBMISSION_QUEUE_ENTRY_FIGURE635_CATTR_PRIOCLASS_URGENT = 0x0, ///< Urgent (Base 2.4, 6.3, Figure 635, page 553, PDF page 583)
	XNVME_SPEC_BASE_SUBMISSION_QUEUE_ENTRY_FIGURE635_CATTR_PRIOCLASS_HIGH = 0x1, ///< High (Base 2.4, 6.3, Figure 635, page 553, PDF page 583)
	XNVME_SPEC_BASE_SUBMISSION_QUEUE_ENTRY_FIGURE635_CATTR_PRIOCLASS_MEDIUM = 0x2, ///< Medium (Base 2.4, 6.3, Figure 635, page 553, PDF page 583)
	XNVME_SPEC_BASE_SUBMISSION_QUEUE_ENTRY_FIGURE635_CATTR_PRIOCLASS_LOW = 0x3, ///< Low (Base 2.4, 6.3, Figure 635, page 553, PDF page 583)
};

/**
 * Connect Command - Data (Base 2.4, 6.3, Figure 636, page 555, PDF page 585)
 */
struct __attribute__((packed)) xnvme_spec_base_data_figure636 {
	uint8_t hostid[16]; ///< Host Identifier, bytes 15:0
	uint16_t cntlid; ///< Controller ID, bytes 17:16
	uint8_t rsvd18[238]; ///< Reserved, bytes 255:18
	uint8_t subnqn[256]; ///< NVM Subsystem NVMe Qualified Name, bytes 511:256
	uint8_t hostnqn[256]; ///< Host NVMe Qualified Name, bytes 767:512
	uint8_t rsvd768[256]; ///< Reserved, bytes 1023:768
};

/**
 * Connect Response (Base 2.4, 6.3, Figure 637, page 556, PDF page 586)
 */
struct __attribute__((packed)) xnvme_spec_base_connect_response {
	uint32_t scs; ///< Status Code Specific, bytes 3:0
	uint32_t rsvd4; ///< Reserved, bytes 7:4
	uint16_t sqhd; ///< SQ Head Pointer, bytes 9:8
	uint16_t rsvd10; ///< Reserved, bytes 11:10
	uint16_t cid; ///< Command Identifier, bytes 13:12
	uint16_t sts; ///< Status Info, bytes 15:14, see XNVME_SPEC_BASE_CONNECT_RESPONSE_STS_*
};

/** Status (Base 2.4, 6.3, Figure 637, page 556, PDF page 586); bits 15:1 of 'sts' */
#define XNVME_SPEC_BASE_CONNECT_RESPONSE_STS_STATUS 0xfffe

/**
 * The values of Status (Base 2.4, 4.2.3.2, Figure 106, page 153, PDF page 179)
 */
enum xnvme_spec_base_connect_response_sts_status {
	XNVME_SPEC_BASE_CONNECT_RESPONSE_STS_STATUS_INCOMPATIBLE_FORMAT = 0x80, ///< Incompatible Format (Base 2.4, 4.2.3.2, Figure 106, page 153, PDF page 179)
	XNVME_SPEC_BASE_CONNECT_RESPONSE_STS_STATUS_CONTROLLER_BUSY = 0x81, ///< Controller Busy (Base 2.4, 4.2.3.2, Figure 106, page 153, PDF page 179)
	XNVME_SPEC_BASE_CONNECT_RESPONSE_STS_STATUS_CONNECT_INVALID_PARAMETERS = 0x82, ///< Connect Invalid Parameters (Base 2.4, 4.2.3.2, Figure 106, page 153, PDF page 179)
	XNVME_SPEC_BASE_CONNECT_RESPONSE_STS_STATUS_CONNECT_RESTART_DISCOVERY = 0x83, ///< Connect Restart Discovery (Base 2.4, 4.2.3.2, Figure 106, page 153, PDF page 179)
	XNVME_SPEC_BASE_CONNECT_RESPONSE_STS_STATUS_CONNECT_INVALID_HOST = 0x84, ///< Connect Invalid Host (Base 2.4, 4.2.3.2, Figure 106, page 153, PDF page 179)
	XNVME_SPEC_BASE_CONNECT_RESPONSE_STS_STATUS_INVALID_QUEUE_TYPE = 0x85, ///< Invalid Queue Type (Base 2.4, 4.2.3.2, Figure 106, page 153, PDF page 179)
	XNVME_SPEC_BASE_CONNECT_RESPONSE_STS_STATUS_DISCOVER_RESTART = 0x90, ///< Discover Restart (Base 2.4, 4.2.3.2, Figure 106, page 153, PDF page 179)
	XNVME_SPEC_BASE_CONNECT_RESPONSE_STS_STATUS_AUTHENTICATION_REQUIRED = 0x91, ///< Authentication Required (Base 2.4, 4.2.3.2, Figure 106, page 153, PDF page 179)
};

/**
 * Disconnect Command - Submission Queue Entry (Base 2.4, 6.4, Figure 639, page 557, PDF page 587)
 */
struct __attribute__((packed)) xnvme_spec_base_submission_queue_entry_figure639 {
	uint32_t cdw0; ///< Command Dword 0, bytes 3:0
	uint8_t fctype; ///< Fabrics Command Type, byte 4
	uint8_t rsvd5[35]; ///< Reserved, bytes 39:5
	uint16_t recfmt; ///< Record Format, bytes 41:40
	uint8_t rsvd42[22]; ///< Reserved, bytes 63:42
};

/**
 * Disconnect Response (Base 2.4, 6.4, Figure 640, page 557, PDF page 587)
 */
struct __attribute__((packed)) xnvme_spec_base_disconnect_response {
	uint64_t rsvd0; ///< Reserved, bytes 7:0
	uint16_t sqhd; ///< SQ Head Pointer, bytes 9:8
	uint16_t rsvd10; ///< Reserved, bytes 11:10
	uint16_t cid; ///< Command Identifier, bytes 13:12
	uint16_t sts; ///< Status Info, bytes 15:14, see XNVME_SPEC_BASE_DISCONNECT_RESPONSE_STS_*
};

/** Status (Base 2.4, 6.4, Figure 640, page 557, PDF page 587); bits 15:1 of 'sts' */
#define XNVME_SPEC_BASE_DISCONNECT_RESPONSE_STS_STATUS 0xfffe

/**
 * The values of Status (Base 2.4, 4.2.3.2, Figure 106, page 153, PDF page 179)
 */
enum xnvme_spec_base_disconnect_response_sts_status {
	XNVME_SPEC_BASE_DISCONNECT_RESPONSE_STS_STATUS_INCOMPATIBLE_FORMAT = 0x80, ///< Incompatible Format (Base 2.4, 4.2.3.2, Figure 106, page 153, PDF page 179)
	XNVME_SPEC_BASE_DISCONNECT_RESPONSE_STS_STATUS_CONTROLLER_BUSY = 0x81, ///< Controller Busy (Base 2.4, 4.2.3.2, Figure 106, page 153, PDF page 179)
	XNVME_SPEC_BASE_DISCONNECT_RESPONSE_STS_STATUS_CONNECT_INVALID_PARAMETERS = 0x82, ///< Connect Invalid Parameters (Base 2.4, 4.2.3.2, Figure 106, page 153, PDF page 179)
	XNVME_SPEC_BASE_DISCONNECT_RESPONSE_STS_STATUS_CONNECT_RESTART_DISCOVERY = 0x83, ///< Connect Restart Discovery (Base 2.4, 4.2.3.2, Figure 106, page 153, PDF page 179)
	XNVME_SPEC_BASE_DISCONNECT_RESPONSE_STS_STATUS_CONNECT_INVALID_HOST = 0x84, ///< Connect Invalid Host (Base 2.4, 4.2.3.2, Figure 106, page 153, PDF page 179)
	XNVME_SPEC_BASE_DISCONNECT_RESPONSE_STS_STATUS_INVALID_QUEUE_TYPE = 0x85, ///< Invalid Queue Type (Base 2.4, 4.2.3.2, Figure 106, page 153, PDF page 179)
	XNVME_SPEC_BASE_DISCONNECT_RESPONSE_STS_STATUS_DISCOVER_RESTART = 0x90, ///< Discover Restart (Base 2.4, 4.2.3.2, Figure 106, page 153, PDF page 179)
	XNVME_SPEC_BASE_DISCONNECT_RESPONSE_STS_STATUS_AUTHENTICATION_REQUIRED = 0x91, ///< Authentication Required (Base 2.4, 4.2.3.2, Figure 106, page 153, PDF page 179)
};

/**
 * Property Get Command - Submission Queue Entry (Base 2.4, 6.5, Figure 641, page 558, PDF page 588)
 */
struct __attribute__((packed)) xnvme_spec_base_submission_queue_entry_figure641 {
	uint32_t cdw0; ///< Command Dword 0, bytes 3:0
	uint8_t fctype; ///< Fabrics Command Type, byte 4
	uint8_t rsvd5[35]; ///< Reserved, bytes 39:5
	uint8_t attrib; ///< Attributes, byte 40, see XNVME_SPEC_BASE_SUBMISSION_QUEUE_ENTRY_FIGURE641_ATTRIB_*
	uint8_t rsvd41[3]; ///< Reserved, bytes 43:41
	uint32_t ofst; ///< Offset, bytes 47:44
	uint8_t rsvd48[16]; ///< Reserved, bytes 63:48
};

/** Property Return Size (Base 2.4, 6.5, Figure 641, page 558, PDF page 588); bits 2:0 of 'attrib' */
#define XNVME_SPEC_BASE_SUBMISSION_QUEUE_ENTRY_FIGURE641_ATTRIB_PRS 0x7

/**
 * Property Get Response (Base 2.4, 6.5, Figure 642, page 558, PDF page 588)
 */
struct __attribute__((packed)) xnvme_spec_base_property_get_response {
	uint64_t value; ///< Value, bytes 7:0
	uint16_t sqhd; ///< SQ Head Pointer, bytes 9:8
	uint16_t rsvd10; ///< Reserved, bytes 11:10
	uint16_t cid; ///< Command Identifier, bytes 13:12
	uint16_t sts; ///< Status Info, bytes 15:14, see XNVME_SPEC_BASE_PROPERTY_GET_RESPONSE_STS_*
};

/** Status (Base 2.4, 6.5, Figure 642, page 558, PDF page 588); bits 15:1 of 'sts' */
#define XNVME_SPEC_BASE_PROPERTY_GET_RESPONSE_STS_STATUS 0xfffe

/**
 * Property Set Command - Submission Queue Entry (Base 2.4, 6.6, Figure 643, page 558, PDF page 588)
 */
struct __attribute__((packed)) xnvme_spec_base_submission_queue_entry_figure643 {
	uint32_t cdw0; ///< Command Dword 0, bytes 3:0
	uint8_t fctype; ///< Fabrics Command Type, byte 4
	uint8_t rsvd5[35]; ///< Reserved, bytes 39:5
	uint8_t attrib; ///< Attributes, byte 40, see XNVME_SPEC_BASE_SUBMISSION_QUEUE_ENTRY_FIGURE643_ATTRIB_*
	uint8_t rsvd41[3]; ///< Reserved, bytes 43:41
	uint32_t ofst; ///< Offset, bytes 47:44
	uint64_t value; ///< Value, bytes 55:48
	uint64_t rsvd56; ///< Reserved, bytes 63:56
};

/** Property Update Size (Base 2.4, 6.6, Figure 643, page 558, PDF page 588); bits 2:0 of 'attrib' */
#define XNVME_SPEC_BASE_SUBMISSION_QUEUE_ENTRY_FIGURE643_ATTRIB_PUS 0x7

/**
 * Property Set Response (Base 2.4, 6.6, Figure 644, page 559, PDF page 589)
 */
struct __attribute__((packed)) xnvme_spec_base_property_set_response {
	uint64_t rsvd0; ///< Reserved, bytes 7:0
	uint16_t sqhd; ///< SQ Head Pointer, bytes 9:8
	uint16_t rsvd10; ///< Reserved, bytes 11:10
	uint16_t cid; ///< Command Identifier, bytes 13:12
	uint16_t sts; ///< Status Info, bytes 15:14, see XNVME_SPEC_BASE_PROPERTY_SET_RESPONSE_STS_*
};

/** Status (Base 2.4, 6.6, Figure 644, page 559, PDF page 589); bits 15:1 of 'sts' */
#define XNVME_SPEC_BASE_PROPERTY_SET_RESPONSE_STS_STATUS 0xfffe

/**
 * Reclaim Unit Handle Status (Base 2.4, 7.3.1.1, Figure 653, page 565, PDF page 595)
 *
 * Followed, from byte 16, by as many Reclaim Unit Handle Status Descriptor as the data says
 */
struct __attribute__((packed)) xnvme_spec_base_reclaim_unit_handle_status {
	uint8_t rsvd0[14]; ///< Reserved, bytes 13:0
	uint16_t nruhsd; ///< Number of Reclaim Unit Handle Status Descriptors, bytes 15:14
};

/**
 * Reservation Acquire (Base 2.4, 7.5, Figure 660, page 568, PDF page 598)
 */
struct __attribute__((packed)) xnvme_spec_base_reservation_acquire {
	uint64_t crkey; ///< Current Reservation Key, bytes 7:0
	uint64_t prkey; ///< Preempt Reservation Key, bytes 15:8
};

/**
 * Reservation Register (Base 2.4, 7.6, Figure 664, page 569, PDF page 599)
 */
struct __attribute__((packed)) xnvme_spec_base_reservation_register {
	uint64_t crkey; ///< Current Reservation Key, bytes 7:0
	uint64_t nrkey; ///< New Reservation Key, bytes 15:8
};

/**
 * Reservation Release (Base 2.4, 7.7, Figure 667, page 570, PDF page 600)
 */
struct __attribute__((packed)) xnvme_spec_base_reservation_release {
	uint64_t crkey; ///< Current Reservation Key, bytes 7:0
};

/**
 * Registrant (Base 2.4, 7.8, Figure 673, page 573, PDF page 603)
 */
struct __attribute__((packed)) xnvme_spec_base_registrant {
	uint16_t cntlid; ///< Controller ID, bytes 1:0
	uint8_t rcsts; ///< Reservation Status, byte 2, see XNVME_SPEC_BASE_REGISTRANT_RCSTS_*
	uint8_t rsvd3[5]; ///< Reserved, bytes 7:3
	uint64_t hostid; ///< Host Identifier, bytes 15:8
	uint64_t rkey; ///< Reservation Key, bytes 23:16
};

/** Association with Host Holding Reservation (Base 2.4, 7.8, Figure 673, page 573, PDF page 603); bit 0 of 'rcsts' */
#define XNVME_SPEC_BASE_REGISTRANT_RCSTS_AHHR 0x1

/**
 * Reservation Status (Base 2.4, 7.8, Figure 671, page 572, PDF page 602)
 *
 * Followed, from byte 24, by as many Registrant Data Structure as the data says, each a 'struct xnvme_spec_base_registrant'
 */
struct __attribute__((packed)) xnvme_spec_base_reservation_status {
	uint32_t gen; ///< Generation, bytes 3:0
	uint8_t rtype; ///< Reservation Type, byte 4
	uint16_t regstrnt; ///< Number of Registrants, bytes 6:5
	uint16_t rsvd7; ///< Reserved, bytes 8:7
	uint8_t ptpls; ///< Persist Through Power Loss State, byte 9
	uint8_t rsvd10[14]; ///< Reserved, bytes 23:10
};

/**
 * The values of Reservation Type (Base 2.4, 7.5, Figure 661, page 568, PDF page 598)
 */
enum xnvme_spec_base_reservation_status_rtype {
	XNVME_SPEC_BASE_RESERVATION_STATUS_RTYPE_WRITE_EXCLUSIVE_RESERVATION = 0x1, ///< Write Exclusive Reservation (Base 2.4, 7.5, Figure 661, page 568, PDF page 598)
	XNVME_SPEC_BASE_RESERVATION_STATUS_RTYPE_EXCLUSIVE_ACCESS_RESERVATION = 0x2, ///< Exclusive Access Reservation (Base 2.4, 7.5, Figure 661, page 568, PDF page 598)
};

/**
 * Reservation Status Extended (Base 2.4, 7.8, Figure 672, page 573, PDF page 603)
 *
 * Followed, from byte 64, by as many Registrant Extended Data Structure n as the data says
 */
struct __attribute__((packed)) xnvme_spec_base_reservation_status_extended {
	uint8_t rshdr[24]; ///< Reservation Status Header, bytes 23:0
	uint8_t rsvd24[40]; ///< Reserved, bytes 63:24
};

/**
 * Registrant Extended (Base 2.4, 7.8, Figure 674, page 573, PDF page 603)
 */
struct __attribute__((packed)) xnvme_spec_base_registrant_extended {
	uint16_t cntlid; ///< Controller ID, bytes 1:0
	uint8_t rcsts; ///< Reservation Status, byte 2
	uint8_t rsvd3[5]; ///< Reserved, bytes 7:3
	uint64_t rkey; ///< Reservation Key, bytes 15:8
	uint8_t hostid[16]; ///< Host Identifier, bytes 31:16
	uint8_t rsvd32[32]; ///< Reserved, bytes 63:32
};

/**
 * CDP Request Data Frame (Base 2.4, 8.1.6.2, Figure 687, page 600, PDF page 630)
 *
 * Followed, from byte 6, by as many Request Data as the data says
 */
struct __attribute__((packed)) xnvme_spec_base_cdp_request_data_frame {
	uint8_t reqdfv; ///< Request Data Frame Version, byte 0
	uint8_t dfhl; ///< Data Frame Header Length, byte 1
	uint16_t rhcps; ///< Request Header and Common Payload Size, bytes 3:2
	uint8_t cdprmt; ///< CDP Request Message Type, byte 4
	uint8_t reqds; ///< Request Data Size, byte 5
};

/**
 * The values of CDP Request Message Type (Base 2.4, 8.1.6.1.1, Figure 686, page 598, PDF page 628)
 */
enum xnvme_spec_base_cdp_request_data_frame_cdprmt {
	XNVME_SPEC_BASE_CDP_REQUEST_DATA_FRAME_CDPRMT_PERSONALITY_UNFREEZE_REQUEST = 0x1, ///< Personality Unfreeze Request (Base 2.4, 8.1.6.1.1, Figure 686, page 598, PDF page 628)
	XNVME_SPEC_BASE_CDP_REQUEST_DATA_FRAME_CDPRMT_PERSONALITY_UNFREEZE_RESPONSE = 0x2, ///< Personality Unfreeze Response (Base 2.4, 8.1.6.1.1, Figure 686, page 598, PDF page 628)
	XNVME_SPEC_BASE_CDP_REQUEST_DATA_FRAME_CDPRMT_DERIVED_HMAC_KEY_PROGRAMMING_REQUEST = 0x3, ///< Derived HMAC Key Programming Request (Base 2.4, 8.1.6.1.1, Figure 686, page 598, PDF page 628)
	XNVME_SPEC_BASE_CDP_REQUEST_DATA_FRAME_CDPRMT_DERIVED_HMAC_KEY_PROGRAMMING_RESPONSE = 0x4, ///< Derived HMAC Key Programming Response (Base 2.4, 8.1.6.1.1, Figure 686, page 598, PDF page 628)
	XNVME_SPEC_BASE_CDP_REQUEST_DATA_FRAME_CDPRMT_RANDOM_NONCE_REQUEST = 0x5, ///< Random Nonce Request (Base 2.4, 8.1.6.1.1, Figure 686, page 598, PDF page 628)
	XNVME_SPEC_BASE_CDP_REQUEST_DATA_FRAME_CDPRMT_RANDOM_NONCE_RESPONSE = 0x6, ///< Random Nonce Response (Base 2.4, 8.1.6.1.1, Figure 686, page 598, PDF page 628)
};

/**
 * CDP Response Data Frame (Base 2.4, 8.1.6.2, Figure 688, page 601, PDF page 631)
 *
 * Followed, from byte 7, by as many Response Data as the data says
 */
struct __attribute__((packed)) xnvme_spec_base_cdp_response_data_frame {
	uint8_t respdfv; ///< Response Data Frame Version, byte 0
	uint8_t dfhl; ///< Data Frame Header Length, byte 1
	uint16_t respdfs; ///< Response Data Frame Size, bytes 3:2
	uint8_t cdprmt; ///< CDP Response Message Type, byte 4
	uint8_t rc; ///< Result Code, byte 5
	uint8_t respds; ///< Response Data Size, byte 6
};

/**
 * The values of CDP Response Message Type (Base 2.4, 8.1.6.1.1, Figure 686, page 598, PDF page 628)
 */
enum xnvme_spec_base_cdp_response_data_frame_cdprmt {
	XNVME_SPEC_BASE_CDP_RESPONSE_DATA_FRAME_CDPRMT_PERSONALITY_UNFREEZE_REQUEST = 0x1, ///< Personality Unfreeze Request (Base 2.4, 8.1.6.1.1, Figure 686, page 598, PDF page 628)
	XNVME_SPEC_BASE_CDP_RESPONSE_DATA_FRAME_CDPRMT_PERSONALITY_UNFREEZE_RESPONSE = 0x2, ///< Personality Unfreeze Response (Base 2.4, 8.1.6.1.1, Figure 686, page 598, PDF page 628)
	XNVME_SPEC_BASE_CDP_RESPONSE_DATA_FRAME_CDPRMT_DERIVED_HMAC_KEY_PROGRAMMING_REQUEST = 0x3, ///< Derived HMAC Key Programming Request (Base 2.4, 8.1.6.1.1, Figure 686, page 598, PDF page 628)
	XNVME_SPEC_BASE_CDP_RESPONSE_DATA_FRAME_CDPRMT_DERIVED_HMAC_KEY_PROGRAMMING_RESPONSE = 0x4, ///< Derived HMAC Key Programming Response (Base 2.4, 8.1.6.1.1, Figure 686, page 598, PDF page 628)
	XNVME_SPEC_BASE_CDP_RESPONSE_DATA_FRAME_CDPRMT_RANDOM_NONCE_REQUEST = 0x5, ///< Random Nonce Request (Base 2.4, 8.1.6.1.1, Figure 686, page 598, PDF page 628)
	XNVME_SPEC_BASE_CDP_RESPONSE_DATA_FRAME_CDPRMT_RANDOM_NONCE_RESPONSE = 0x6, ///< Random Nonce Response (Base 2.4, 8.1.6.1.1, Figure 686, page 598, PDF page 628)
};

/**
 * CDP Programmable Key Authentication Unfreeze Request Data (Base 2.4, 8.1.6.2.1.1.1, Figure 689, page 601, PDF page 631)
 *
 * Followed, from byte 11, by as many Random Nonce as the data says
 *
 * Not members:
 * HMAC Size: at an offset the data gives, in 'random_nonce'
 * HMAC Value: at an offset the data gives, in 'random_nonce'
 */
struct __attribute__((packed)) xnvme_spec_base_cdp_programmable_key_authentication_unfreeze_request_data {
	uint8_t cdpreqdfcf[6]; ///< CDP Request Data Frame Common Fields, bytes 5:0
	uint8_t perid; ///< Personality Identifier, byte 6
	uint8_t cdpami; ///< CDP Authentication Method Identifier, byte 7
	uint8_t cdpaa; ///< CDP Authentication Algorithm, byte 8
	uint16_t randns; ///< Random Nonce Size, bytes 10:9
};

/**
 * The values of Personality Identifier (Base 2.4, 5.2.30.1.25, Figure 512, page 483, PDF page 511)
 */
enum xnvme_spec_base_cdp_programmable_key_authentication_unfreeze_request_data_perid {
	XNVME_SPEC_BASE_CDP_PROGRAMMABLE_KEY_AUTHENTICATION_UNFREEZE_REQUEST_DATA_PERID_MANUFACTURING_DEFAULT_PERSONALITY = 0x0, ///< Manufacturing Default Personality (Base 2.4, 5.2.30.1.25, Figure 512, page 483, PDF page 511)
	XNVME_SPEC_BASE_CDP_PROGRAMMABLE_KEY_AUTHENTICATION_UNFREEZE_REQUEST_DATA_PERID_SECURITY_PERSONALITY = 0x1, ///< Security Personality (Base 2.4, 5.2.30.1.25, Figure 512, page 483, PDF page 511)
	XNVME_SPEC_BASE_CDP_PROGRAMMABLE_KEY_AUTHENTICATION_UNFREEZE_REQUEST_DATA_PERID_LOCKDOWN_PERSISTENCE_PERSONALITY = 0x2, ///< Lockdown Persistence Personality (Base 2.4, 5.2.30.1.25, Figure 512, page 483, PDF page 511)
	XNVME_SPEC_BASE_CDP_PROGRAMMABLE_KEY_AUTHENTICATION_UNFREEZE_REQUEST_DATA_PERID_ALL_PERSONALITIES = 0xff, ///< All Personalities (Base 2.4, 5.2.30.1.25, Figure 512, page 483, PDF page 511)
};

/**
 * CDP Programmable Key Authentication Unfreeze Response Data (Base 2.4, 8.1.6.2.1.1.2, Figure 690, page 602, PDF page 632)
 */
struct __attribute__((packed)) xnvme_spec_base_cdp_programmable_key_authentication_unfreeze_response_data {
	uint8_t cdprespdfcf[7]; ///< CDP Response Data Frame Common Fields, bytes 6:0
	uint8_t perid; ///< Personality Identifier, byte 7
	uint8_t cdpami; ///< CDP Authentication Method Identifier, byte 8
	uint8_t cdpaa; ///< CDP Authentication Algorithm, byte 9
};

/**
 * The values of Personality Identifier (Base 2.4, 5.2.30.1.25, Figure 512, page 483, PDF page 511)
 */
enum xnvme_spec_base_cdp_programmable_key_authentication_unfreeze_response_data_perid {
	XNVME_SPEC_BASE_CDP_PROGRAMMABLE_KEY_AUTHENTICATION_UNFREEZE_RESPONSE_DATA_PERID_MANUFACTURING_DEFAULT_PERSONALITY = 0x0, ///< Manufacturing Default Personality (Base 2.4, 5.2.30.1.25, Figure 512, page 483, PDF page 511)
	XNVME_SPEC_BASE_CDP_PROGRAMMABLE_KEY_AUTHENTICATION_UNFREEZE_RESPONSE_DATA_PERID_SECURITY_PERSONALITY = 0x1, ///< Security Personality (Base 2.4, 5.2.30.1.25, Figure 512, page 483, PDF page 511)
	XNVME_SPEC_BASE_CDP_PROGRAMMABLE_KEY_AUTHENTICATION_UNFREEZE_RESPONSE_DATA_PERID_LOCKDOWN_PERSISTENCE_PERSONALITY = 0x2, ///< Lockdown Persistence Personality (Base 2.4, 5.2.30.1.25, Figure 512, page 483, PDF page 511)
	XNVME_SPEC_BASE_CDP_PROGRAMMABLE_KEY_AUTHENTICATION_UNFREEZE_RESPONSE_DATA_PERID_ALL_PERSONALITIES = 0xff, ///< All Personalities (Base 2.4, 5.2.30.1.25, Figure 512, page 483, PDF page 511)
};

/**
 * CDP Physical Credential Authentication Unfreeze Request Data (Base 2.4, 8.1.6.2.1.2.1, Figure 691, page 602, PDF page 632)
 *
 * Followed, from byte 8, by as many Credential Value as the data says
 */
struct __attribute__((packed)) xnvme_spec_base_cdp_physical_credential_authentication_unfreeze_request_data {
	uint8_t cdpreqdfcf[6]; ///< CDP Request Data Frame Common Fields, bytes 5:0
	uint8_t perid; ///< Personality Identifier, byte 6
	uint8_t cdpami; ///< CDP Authentication Method Identifier, byte 7
};

/**
 * The values of Personality Identifier (Base 2.4, 5.2.30.1.25, Figure 512, page 483, PDF page 511)
 */
enum xnvme_spec_base_cdp_physical_credential_authentication_unfreeze_request_data_perid {
	XNVME_SPEC_BASE_CDP_PHYSICAL_CREDENTIAL_AUTHENTICATION_UNFREEZE_REQUEST_DATA_PERID_MANUFACTURING_DEFAULT_PERSONALITY = 0x0, ///< Manufacturing Default Personality (Base 2.4, 5.2.30.1.25, Figure 512, page 483, PDF page 511)
	XNVME_SPEC_BASE_CDP_PHYSICAL_CREDENTIAL_AUTHENTICATION_UNFREEZE_REQUEST_DATA_PERID_SECURITY_PERSONALITY = 0x1, ///< Security Personality (Base 2.4, 5.2.30.1.25, Figure 512, page 483, PDF page 511)
	XNVME_SPEC_BASE_CDP_PHYSICAL_CREDENTIAL_AUTHENTICATION_UNFREEZE_REQUEST_DATA_PERID_LOCKDOWN_PERSISTENCE_PERSONALITY = 0x2, ///< Lockdown Persistence Personality (Base 2.4, 5.2.30.1.25, Figure 512, page 483, PDF page 511)
	XNVME_SPEC_BASE_CDP_PHYSICAL_CREDENTIAL_AUTHENTICATION_UNFREEZE_REQUEST_DATA_PERID_ALL_PERSONALITIES = 0xff, ///< All Personalities (Base 2.4, 5.2.30.1.25, Figure 512, page 483, PDF page 511)
};

/**
 * CDP Physical Authentication Response Data (Base 2.4, 8.1.6.2.1.2.2, Figure 692, page 603, PDF page 633)
 */
struct __attribute__((packed)) xnvme_spec_base_cdp_physical_authentication_response_data {
	uint8_t cdprespdfcf[7]; ///< CDP Response Data Frame Common Fields, bytes 6:0
	uint8_t perid; ///< Personality Identifier, byte 7
	uint8_t cdpami; ///< CDP Authentication Method Identifier, byte 8
};

/**
 * The values of Personality Identifier (Base 2.4, 5.2.30.1.25, Figure 512, page 483, PDF page 511)
 */
enum xnvme_spec_base_cdp_physical_authentication_response_data_perid {
	XNVME_SPEC_BASE_CDP_PHYSICAL_AUTHENTICATION_RESPONSE_DATA_PERID_MANUFACTURING_DEFAULT_PERSONALITY = 0x0, ///< Manufacturing Default Personality (Base 2.4, 5.2.30.1.25, Figure 512, page 483, PDF page 511)
	XNVME_SPEC_BASE_CDP_PHYSICAL_AUTHENTICATION_RESPONSE_DATA_PERID_SECURITY_PERSONALITY = 0x1, ///< Security Personality (Base 2.4, 5.2.30.1.25, Figure 512, page 483, PDF page 511)
	XNVME_SPEC_BASE_CDP_PHYSICAL_AUTHENTICATION_RESPONSE_DATA_PERID_LOCKDOWN_PERSISTENCE_PERSONALITY = 0x2, ///< Lockdown Persistence Personality (Base 2.4, 5.2.30.1.25, Figure 512, page 483, PDF page 511)
	XNVME_SPEC_BASE_CDP_PHYSICAL_AUTHENTICATION_RESPONSE_DATA_PERID_ALL_PERSONALITIES = 0xff, ///< All Personalities (Base 2.4, 5.2.30.1.25, Figure 512, page 483, PDF page 511)
};

/**
 * CDP Derived HMAC Key Programming Request Data (Base 2.4, 8.1.6.2.1.3.1, Figure 693, page 603, PDF page 633)
 *
 * Followed, from byte 8, by as many Host Programmed Key as the data says
 */
struct __attribute__((packed)) xnvme_spec_base_cdp_derived_hmac_key_programming_request_data {
	uint8_t cdprdfcf[6]; ///< CDP Request Data Frame Common Fields, bytes 5:0
	uint8_t cdpaa; ///< CDP Authentication Algorithm, byte 6
	uint8_t hpks; ///< Host Programmed Key Size, byte 7
};

/**
 * CDP Derived HMAC Key Programming Response Data (Base 2.4, 8.1.6.2.1.3.2, Figure 694, page 603, PDF page 634)
 */
struct __attribute__((packed)) xnvme_spec_base_cdp_derived_hmac_key_programming_response_data {
	uint8_t cdprespdfcf[7]; ///< CDP Response Data Frame Common Fields, bytes 6:0
};

/**
 * CDP Random Nonce Request Data (Base 2.4, 8.1.6.2.1.4.1, Figure 695, page 604, PDF page 634)
 */
struct __attribute__((packed)) xnvme_spec_base_cdp_random_nonce_request_data {
	uint8_t cdpreqdfcf[6]; ///< CDP Request Data Frame Common Fields, bytes 5:0
};

/**
 * CDP Random Nonce Read Response Data Frame (Base 2.4, 8.1.6.2.1.4.2, Figure 696, page 604, PDF page 634)
 *
 * Followed, from byte 9, by as many Random Nonce as the data says
 */
struct __attribute__((packed)) xnvme_spec_base_cdp_random_nonce_read_response_data_frame {
	uint8_t cdprespdfcf[7]; ///< CDP Response Data Frame Common Fields, bytes 6:0
	uint16_t randns; ///< Random Nonce Size, bytes 8:7
};

/**
 * CDP Subsystem Unique String Definition (Base 2.4, 8.1.6.3.1.1, Figure 698, page 607, PDF page 637)
 */
struct __attribute__((packed)) xnvme_spec_base_cdp_subsystem_unique_string_definition {
	uint8_t nqnid[220]; ///< NQN Substring, bytes 219:0
	uint8_t fguid[16]; ///< FGUID, bytes 235:220
	uint8_t sn[20]; ///< Serial Number, bytes 255:236
};

/**
 * Identify Directive - Return Parameters (Base 2.4, 8.1.9.2.1.1, Figure 705, page 614, PDF page 644)
 */
struct __attribute__((packed)) xnvme_spec_base_return_parameters {
	uint8_t directives_supported[32]; ///< Directives Supported, bytes 31:0
	uint8_t directives_enabled[32]; ///< Directives Enabled, bytes 63:32
	uint8_t directive_persistent_across_controller_level_resets[32]; ///< Directive Persistent Across Controller Level Resets, bytes 95:64
	uint8_t rsvd96[4000]; ///< Reserved, bytes 4095:96
};

/**
 * Streams Directive - Return Parameters (Base 2.4, 8.1.9.3.1.1, Figure 712, page 620, PDF page 650)
 */
struct __attribute__((packed)) xnvme_spec_base_return_parameters_figure712 {
	uint16_t msl; ///< Max Streams Limit, bytes 1:0
	uint16_t nssa; ///< NVM Subsystem Streams Available, bytes 3:2
	uint16_t nsso; ///< NVM Subsystem Streams Open, bytes 5:4
	uint8_t nssc; ///< NVM Subsystem Stream Capability, byte 6, see XNVME_SPEC_BASE_RETURN_PARAMETERS_FIGURE712_NSSC_*
	uint8_t rsvd7[9]; ///< Reserved, bytes 15:7
	uint32_t sws; ///< Stream Write Size, bytes 19:16
	uint16_t sgs; ///< Stream Granularity Size, bytes 21:20
	uint16_t nsa; ///< Namespace Streams Allocated, bytes 23:22
	uint16_t nso; ///< Namespace Streams Open, bytes 25:24
	uint8_t rsvd26[6]; ///< Reserved, bytes 31:26
};

/** Streams Require Non-Zero Host Identifier (Base 2.4, 8.1.9.3.1.1, Figure 712, page 620, PDF page 650); bit 1 of 'nssc' */
#define XNVME_SPEC_BASE_RETURN_PARAMETERS_FIGURE712_NSSC_SRNZID 0x2
/** Shared Stream Identifiers (Base 2.4, 8.1.9.3.1.1, Figure 712, page 620, PDF page 650); bit 0 of 'nssc' */
#define XNVME_SPEC_BASE_RETURN_PARAMETERS_FIGURE712_NSSC_SSID 0x1

/**
 * Streams Directive - Get Status (Base 2.4, 8.1.9.3.1.2, Figure 713, page 622, PDF page 652)
 */
struct __attribute__((packed)) xnvme_spec_base_get_status {
	uint16_t osc; ///< Open Stream Count, bytes 1:0
	uint16_t sid[65535]; ///< Stream Identifier, 65535 of 2 bytes, bytes 131071:2
};

/**
 * RPMB Device Configuration Block (Base 2.4, 8.1.24, Figure 756, page 687, PDF page 717)
 */
struct __attribute__((packed)) xnvme_spec_base_rpmb_device_configuration_block {
	uint8_t bppee; ///< Boot Partition Protection Enable, byte 0, see XNVME_SPEC_BASE_RPMB_DEVICE_CONFIGURATION_BLOCK_BPPEE_*
	uint8_t bpls; ///< Boot Partition Protection State, byte 1, see XNVME_SPEC_BASE_RPMB_DEVICE_CONFIGURATION_BLOCK_BPLS_*
	uint8_t wpc; ///< Write Protection Control, byte 2, see XNVME_SPEC_BASE_RPMB_DEVICE_CONFIGURATION_BLOCK_WPC_*
	uint8_t rsvd3[509]; ///< Reserved, bytes 511:3
};

/** Boot Partition Write Protection Enabled (Base 2.4, 8.1.24, Figure 756, page 687, PDF page 717); bit 0 of 'bppee' */
#define XNVME_SPEC_BASE_RPMB_DEVICE_CONFIGURATION_BLOCK_BPPEE_BPPED 0x1
/** Boot Partition 1 Write Locked (Base 2.4, 8.1.24, Figure 756, page 687, PDF page 717); bit 1 of 'bpls' */
#define XNVME_SPEC_BASE_RPMB_DEVICE_CONFIGURATION_BLOCK_BPLS_BPP1L 0x2
/** Boot Partition 0 Write Locked (Base 2.4, 8.1.24, Figure 756, page 687, PDF page 717); bit 0 of 'bpls' */
#define XNVME_SPEC_BASE_RPMB_DEVICE_CONFIGURATION_BLOCK_BPLS_BPP0L 0x1
/** Permanent Write Protect Control (Base 2.4, 8.1.24, Figure 756, page 687, PDF page 717); bit 1 of 'wpc' */
#define XNVME_SPEC_BASE_RPMB_DEVICE_CONFIGURATION_BLOCK_WPC_PWPC 0x2
/** Write Protect Until Power Cycle Control (Base 2.4, 8.1.24, Figure 756, page 687, PDF page 717); bit 0 of 'wpc' */
#define XNVME_SPEC_BASE_RPMB_DEVICE_CONFIGURATION_BLOCK_WPC_WPUPCC 0x1

/**
 * ZoneGroup Detailed Representation (Base 2.4, 8.3.2.3.2, Figure 808, page 767, PDF page 797)
 *
 * Followed, from byte 256, by as many Zone as the data says
 *
 * Not members:
 * Zone 2: at an offset the data gives, in 'zone'
 * Zone N: at an offset the data gives, in 'zone'
 */
struct __attribute__((packed)) xnvme_spec_base_zonegroup_detailed_representation {
	uint8_t zgorig[224]; ///< ZoneGroup Originator, bytes 223:0
	uint8_t zgname[30]; ///< ZoneGroup Name, bytes 253:224
	uint16_t numz; ///< Number of Zones, bytes 255:254
};

/**
 * Zone Detailed Representation (Base 2.4, 8.3.2.3.3, Figure 809, page 768, PDF page 798)
 *
 * Followed, from byte 128, by as many Zone Member as the data says
 *
 * Not members:
 * Zone Property 1: at an offset the data gives, in 'zm'
 * Zone Property 2: at an offset the data gives, in 'zm'
 * Zone Property k: at an offset the data gives, in 'zm'
 */
struct __attribute__((packed)) xnvme_spec_base_zone_detailed_representation {
	uint8_t zname[124]; ///< Zone Name, bytes 123:0
	uint16_t numzm; ///< Number of Zone Members, bytes 125:124
	uint16_t numzp; ///< Number of Zone Properties, bytes 127:126
};

/**
 * {NQN, Role} Zone Member Format (Base 2.4, 8.3.2.3.4.2, Figure 811, page 769, PDF page 799)
 */
struct __attribute__((packed)) xnvme_spec_base_nqn_role_zone_member_format {
	uint8_t zmtype; ///< Zone Member Type, byte 0
	uint16_t zmlen; ///< Zone Member Length, bytes 2:1
	uint8_t zmrole; ///< Zone Member Role, byte 3
	uint8_t rsvd4[28]; ///< Reserved, bytes 31:4
	uint8_t zmnqn[224]; ///< Zone Member NQN, bytes 255:32
};

/**
 * The values of Zone Member Role (Base 2.4, 8.3.2.3.4.2, Figure 812, page 769, PDF page 799)
 */
enum xnvme_spec_base_nqn_role_zone_member_format_zmrole {
	XNVME_SPEC_BASE_NQN_ROLE_ZONE_MEMBER_FORMAT_ZMROLE_HOST = 0x1, ///< Host (Base 2.4, 8.3.2.3.4.2, Figure 812, page 769, PDF page 799)
};

/**
 * {(NQN+IP+Protocol), Role} Zone Member Format (Base 2.4, 8.3.2.3.4.3, Figure 813, page 770, PDF page 800)
 */
struct __attribute__((packed)) xnvme_spec_base_nqn_ip_protocol_role_zone_member_format {
	uint8_t zmtype; ///< Zone Member Type, byte 0
	uint16_t zmlen; ///< Zone Member Length, bytes 2:1
	uint8_t zmrole; ///< Zone Member Role, byte 3
	uint8_t zmadrfam; ///< Zone Member Address Family, byte 4
	uint8_t zmipproto; ///< Zone Member IP Protocol, byte 5
	uint16_t rsvd6; ///< Reserved, bytes 7:6
	uint8_t zmtraddr[16]; ///< Zone Member Transport Address, bytes 23:8
	uint64_t rsvd24; ///< Reserved, bytes 31:24
	uint8_t zmnqn[224]; ///< Zone Member NQN, bytes 255:32
};

/**
 * The values of Zone Member Role (Base 2.4, 8.3.2.3.4.2, Figure 812, page 769, PDF page 799)
 */
enum xnvme_spec_base_nqn_ip_protocol_role_zone_member_format_zmrole {
	XNVME_SPEC_BASE_NQN_IP_PROTOCOL_ROLE_ZONE_MEMBER_FORMAT_ZMROLE_HOST = 0x1, ///< Host (Base 2.4, 8.3.2.3.4.2, Figure 812, page 769, PDF page 799)
};

/**
 * The values of Zone Member IP Protocol (Base 2.4, 8.3.2.3.4.3, Figure 815, page 770, PDF page 800)
 */
enum xnvme_spec_base_nqn_ip_protocol_role_zone_member_format_zmipproto {
	XNVME_SPEC_BASE_NQN_IP_PROTOCOL_ROLE_ZONE_MEMBER_FORMAT_ZMIPPROTO_TCP = 0x6, ///< TCP (Base 2.4, 8.3.2.3.4.3, Figure 815, page 770, PDF page 800)
	XNVME_SPEC_BASE_NQN_IP_PROTOCOL_ROLE_ZONE_MEMBER_FORMAT_ZMIPPROTO_UDP = 0x11, ///< UDP (Base 2.4, 8.3.2.3.4.3, Figure 815, page 770, PDF page 800)
};

/**
 * {(NQN+IP+Protocol+IP Protocol Port), Role} Zone Member Format (Base 2.4, 8.3.2.3.4.4, Figure 816, page 771, PDF page 801)
 */
struct __attribute__((packed)) xnvme_spec_base_nqn_ip_protocol_ip_protocol_port_role_zone_member_format {
	uint8_t zmtype; ///< Zone Member Type, byte 0
	uint16_t zmlen; ///< Zone Member Length, bytes 2:1
	uint8_t zmrole; ///< Zone Member Role, byte 3
	uint8_t zmadrfam; ///< Zone Member Address Family, byte 4
	uint8_t zmipproto; ///< Zone Member IP Protocol, byte 5
	uint16_t zmtrsvcid; ///< Zone Member Transport Service Identifier, bytes 7:6
	uint8_t zmtraddr[16]; ///< Zone Member Transport Address, bytes 23:8
	uint64_t rsvd24; ///< Reserved, bytes 31:24
	uint8_t zmnqn[224]; ///< Zone Member NQN, bytes 255:32
};

/**
 * The values of Zone Member Role (Base 2.4, 8.3.2.3.4.2, Figure 812, page 769, PDF page 799)
 */
enum xnvme_spec_base_nqn_ip_protocol_ip_protocol_port_role_zone_member_format_zmrole {
	XNVME_SPEC_BASE_NQN_IP_PROTOCOL_IP_PROTOCOL_PORT_ROLE_ZONE_MEMBER_FORMAT_ZMROLE_HOST = 0x1, ///< Host (Base 2.4, 8.3.2.3.4.2, Figure 812, page 769, PDF page 799)
};

/**
 * The values of Zone Member IP Protocol (Base 2.4, 8.3.2.3.4.3, Figure 815, page 770, PDF page 800)
 */
enum xnvme_spec_base_nqn_ip_protocol_ip_protocol_port_role_zone_member_format_zmipproto {
	XNVME_SPEC_BASE_NQN_IP_PROTOCOL_IP_PROTOCOL_PORT_ROLE_ZONE_MEMBER_FORMAT_ZMIPPROTO_TCP = 0x6, ///< TCP (Base 2.4, 8.3.2.3.4.3, Figure 815, page 770, PDF page 800)
	XNVME_SPEC_BASE_NQN_IP_PROTOCOL_IP_PROTOCOL_PORT_ROLE_ZONE_MEMBER_FORMAT_ZMIPPROTO_UDP = 0x11, ///< UDP (Base 2.4, 8.3.2.3.4.3, Figure 815, page 770, PDF page 800)
};

/**
 * {(NQN+IP+Protocol+IP Protocol Port+PortID), Role} Zone Member Format (Base 2.4, 8.3.2.3.4.5, Figure 817, page 771, PDF page 801)
 */
struct __attribute__((packed)) xnvme_spec_base_nqn_ip_protocol_ip_protocol_port_portid_role_zone_member_format {
	uint8_t zmtype; ///< Zone Member Type, byte 0
	uint16_t zmlen; ///< Zone Member Length, bytes 2:1
	uint8_t zmrole; ///< Zone Member Role, byte 3
	uint8_t zmadrfam; ///< Zone Member Address Family, byte 4
	uint8_t zmipproto; ///< Zone Member IP Protocol, byte 5
	uint16_t zmtrsvcid; ///< Zone Member Transport Service Identifier, bytes 7:6
	uint8_t zmtraddr[16]; ///< Zone Member Transport Address, bytes 23:8
	uint16_t zmportid; ///< Zone Member PortID, bytes 25:24
	uint8_t rsvd26[6]; ///< Reserved, bytes 31:26
	uint8_t zmnqn[224]; ///< Zone Member NQN, bytes 255:32
};

/**
 * The values of Zone Member Role (Base 2.4, 8.3.2.3.4.2, Figure 812, page 769, PDF page 799)
 */
enum xnvme_spec_base_nqn_ip_protocol_ip_protocol_port_portid_role_zone_member_format_zmrole {
	XNVME_SPEC_BASE_NQN_IP_PROTOCOL_IP_PROTOCOL_PORT_PORTID_ROLE_ZONE_MEMBER_FORMAT_ZMROLE_HOST = 0x1, ///< Host (Base 2.4, 8.3.2.3.4.2, Figure 812, page 769, PDF page 799)
};

/**
 * The values of Zone Member IP Protocol (Base 2.4, 8.3.2.3.4.3, Figure 815, page 770, PDF page 800)
 */
enum xnvme_spec_base_nqn_ip_protocol_ip_protocol_port_portid_role_zone_member_format_zmipproto {
	XNVME_SPEC_BASE_NQN_IP_PROTOCOL_IP_PROTOCOL_PORT_PORTID_ROLE_ZONE_MEMBER_FORMAT_ZMIPPROTO_TCP = 0x6, ///< TCP (Base 2.4, 8.3.2.3.4.3, Figure 815, page 770, PDF page 800)
	XNVME_SPEC_BASE_NQN_IP_PROTOCOL_IP_PROTOCOL_PORT_PORTID_ROLE_ZONE_MEMBER_FORMAT_ZMIPPROTO_UDP = 0x11, ///< UDP (Base 2.4, 8.3.2.3.4.3, Figure 815, page 770, PDF page 800)
};

/**
 * {(NQN+PortID), Role} Zone Member Format (Base 2.4, 8.3.2.3.4.6, Figure 818, page 772, PDF page 802)
 */
struct __attribute__((packed)) xnvme_spec_base_nqn_portid_role_zone_member_format {
	uint8_t zmtype; ///< Zone Member Type, byte 0
	uint16_t zmlen; ///< Zone Member Length, bytes 2:1
	uint8_t zmrole; ///< Zone Member Role, byte 3
	uint16_t zmportid; ///< Zone Member PortID, bytes 5:4
	uint8_t rsvd6[26]; ///< Reserved, bytes 31:6
	uint8_t zmnqn[224]; ///< Zone Member NQN, bytes 255:32
};

/**
 * The values of Zone Member Role (Base 2.4, 8.3.2.3.4.2, Figure 812, page 769, PDF page 799)
 */
enum xnvme_spec_base_nqn_portid_role_zone_member_format_zmrole {
	XNVME_SPEC_BASE_NQN_PORTID_ROLE_ZONE_MEMBER_FORMAT_ZMROLE_HOST = 0x1, ///< Host (Base 2.4, 8.3.2.3.4.2, Figure 812, page 769, PDF page 799)
};

/**
 * {(NQN+Protocol+PortID+ADRFAM), Role} Zone Member Format (Base 2.4, 8.3.2.3.4.7, Figure 819, page 772, PDF page 802)
 */
struct __attribute__((packed)) xnvme_spec_base_nqn_protocol_portid_adrfam_role_zone_member_format {
	uint8_t zmtype; ///< Zone Member Type, byte 0
	uint16_t zmlen; ///< Zone Member Length, bytes 2:1
	uint8_t zmrole; ///< Zone Member Role, byte 3
	uint8_t zmadrfam; ///< Zone Member Address Family, byte 4
	uint8_t zmipproto; ///< Zone Member IP Protocol, byte 5
	uint16_t zmportid; ///< Zone Member PortID, bytes 7:6
	uint8_t rsvd8[24]; ///< Reserved, bytes 31:8
	uint8_t zmnqn[224]; ///< Zone Member NQN, bytes 255:32
};

/**
 * The values of Zone Member Role (Base 2.4, 8.3.2.3.4.2, Figure 812, page 769, PDF page 799)
 */
enum xnvme_spec_base_nqn_protocol_portid_adrfam_role_zone_member_format_zmrole {
	XNVME_SPEC_BASE_NQN_PROTOCOL_PORTID_ADRFAM_ROLE_ZONE_MEMBER_FORMAT_ZMROLE_HOST = 0x1, ///< Host (Base 2.4, 8.3.2.3.4.2, Figure 812, page 769, PDF page 799)
};

/**
 * The values of Zone Member IP Protocol (Base 2.4, 8.3.2.3.4.3, Figure 815, page 770, PDF page 800)
 */
enum xnvme_spec_base_nqn_protocol_portid_adrfam_role_zone_member_format_zmipproto {
	XNVME_SPEC_BASE_NQN_PROTOCOL_PORTID_ADRFAM_ROLE_ZONE_MEMBER_FORMAT_ZMIPPROTO_TCP = 0x6, ///< TCP (Base 2.4, 8.3.2.3.4.3, Figure 815, page 770, PDF page 800)
	XNVME_SPEC_BASE_NQN_PROTOCOL_PORTID_ADRFAM_ROLE_ZONE_MEMBER_FORMAT_ZMIPPROTO_UDP = 0x11, ///< UDP (Base 2.4, 8.3.2.3.4.3, Figure 815, page 770, PDF page 800)
};

/**
 * {(IP+Protocol), Role} Zone Member Format (Base 2.4, 8.3.2.3.4.8, Figure 820, page 772, PDF page 802)
 */
struct __attribute__((packed)) xnvme_spec_base_ip_protocol_role_zone_member_format {
	uint8_t zmtype; ///< Zone Member Type, byte 0
	uint16_t zmlen; ///< Zone Member Length, bytes 2:1
	uint8_t zmrole; ///< Zone Member Role, byte 3
	uint8_t zmadrfam; ///< Zone Member Address Family, byte 4
	uint8_t zmipproto; ///< Zone Member IP Protocol, byte 5
	uint16_t rsvd6; ///< Reserved, bytes 7:6
	uint8_t zmtraddr[16]; ///< Zone Member Transport Address, bytes 23:8
	uint8_t rsvd24[232]; ///< Reserved, bytes 255:24
};

/**
 * The values of Zone Member Role (Base 2.4, 8.3.2.3.4.2, Figure 812, page 769, PDF page 799)
 */
enum xnvme_spec_base_ip_protocol_role_zone_member_format_zmrole {
	XNVME_SPEC_BASE_IP_PROTOCOL_ROLE_ZONE_MEMBER_FORMAT_ZMROLE_HOST = 0x1, ///< Host (Base 2.4, 8.3.2.3.4.2, Figure 812, page 769, PDF page 799)
};

/**
 * The values of Zone Member IP Protocol (Base 2.4, 8.3.2.3.4.3, Figure 815, page 770, PDF page 800)
 */
enum xnvme_spec_base_ip_protocol_role_zone_member_format_zmipproto {
	XNVME_SPEC_BASE_IP_PROTOCOL_ROLE_ZONE_MEMBER_FORMAT_ZMIPPROTO_TCP = 0x6, ///< TCP (Base 2.4, 8.3.2.3.4.3, Figure 815, page 770, PDF page 800)
	XNVME_SPEC_BASE_IP_PROTOCOL_ROLE_ZONE_MEMBER_FORMAT_ZMIPPROTO_UDP = 0x11, ///< UDP (Base 2.4, 8.3.2.3.4.3, Figure 815, page 770, PDF page 800)
};

/**
 * {(IP+Protocol+IP Protocol Port), Role} Zone Member Format (Base 2.4, 8.3.2.3.4.9, Figure 821, page 773, PDF page 803)
 */
struct __attribute__((packed)) xnvme_spec_base_ip_protocol_ip_protocol_port_role_zone_member_format {
	uint8_t zmtype; ///< Zone Member Type, byte 0
	uint16_t zmlen; ///< Zone Member Length, bytes 2:1
	uint8_t zmrole; ///< Zone Member Role, byte 3
	uint8_t zmadrfam; ///< Zone Member Address Family, byte 4
	uint8_t zmipproto; ///< Zone Member IP Protocol, byte 5
	uint16_t zmtrsvcid; ///< Zone Member Transport Service Identifier, bytes 7:6
	uint8_t zmtraddr[16]; ///< Zone Member Transport Address, bytes 23:8
	uint8_t rsvd24[232]; ///< Reserved, bytes 255:24
};

/**
 * The values of Zone Member Role (Base 2.4, 8.3.2.3.4.2, Figure 812, page 769, PDF page 799)
 */
enum xnvme_spec_base_ip_protocol_ip_protocol_port_role_zone_member_format_zmrole {
	XNVME_SPEC_BASE_IP_PROTOCOL_IP_PROTOCOL_PORT_ROLE_ZONE_MEMBER_FORMAT_ZMROLE_HOST = 0x1, ///< Host (Base 2.4, 8.3.2.3.4.2, Figure 812, page 769, PDF page 799)
};

/**
 * The values of Zone Member IP Protocol (Base 2.4, 8.3.2.3.4.3, Figure 815, page 770, PDF page 800)
 */
enum xnvme_spec_base_ip_protocol_ip_protocol_port_role_zone_member_format_zmipproto {
	XNVME_SPEC_BASE_IP_PROTOCOL_IP_PROTOCOL_PORT_ROLE_ZONE_MEMBER_FORMAT_ZMIPPROTO_TCP = 0x6, ///< TCP (Base 2.4, 8.3.2.3.4.3, Figure 815, page 770, PDF page 800)
	XNVME_SPEC_BASE_IP_PROTOCOL_IP_PROTOCOL_PORT_ROLE_ZONE_MEMBER_FORMAT_ZMIPPROTO_UDP = 0x11, ///< UDP (Base 2.4, 8.3.2.3.4.3, Figure 815, page 770, PDF page 800)
};

/**
 * {ZoneAlias} Zone Member Format (Base 2.4, 8.3.2.3.4.10, Figure 822, page 774, PDF page 804)
 */
struct __attribute__((packed)) xnvme_spec_base_zonealias_zone_member_format {
	uint8_t zmtype; ///< Zone Member Type, byte 0
	uint16_t zmlen; ///< Zone Member Length, bytes 2:1
	uint8_t rsvd3[29]; ///< Reserved, bytes 31:3
	uint8_t zmzaname[124]; ///< Zone Member ZoneAlias Name, bytes 155:32
	uint8_t rsvd156[100]; ///< Reserved, bytes 255:156
};

/**
 * Fabric Enforced Zone Property Format (Base 2.4, 8.3.2.3.5.2, Figure 824, page 774, PDF page 804)
 */
struct __attribute__((packed)) xnvme_spec_base_fabric_enforced_zone_property_format {
	uint8_t zptype; ///< Zone Property Type, byte 0
	uint16_t zplen; ///< Zone Property Length, bytes 2:1
	uint8_t zpval; ///< Zone Property Value, byte 3
};

/**
 * ZoneAlias Detailed Representation (Base 2.4, 8.3.2.3.6, Figure 825, page 774, PDF page 804)
 *
 * Followed, from byte 128, by as many ZoneAlias Member as the data says
 */
struct __attribute__((packed)) xnvme_spec_base_zonealias_detailed_representation {
	uint8_t zaname[124]; ///< ZoneAlias Name, bytes 123:0
	uint16_t numzam; ///< Number of ZoneAlias Members, bytes 125:124
	uint16_t rsvd126; ///< Reserved, bytes 127:126
};

/**
 * FZL Data for GAZ (Base 2.4, 8.3.2.3.8.2.1, Figure 827, page 776, PDF page 806)
 */
struct __attribute__((packed)) xnvme_spec_base_fzl_data_for_gaz {
	uint8_t otyp; ///< Operation Type, byte 0
	uint8_t rsvd1[3]; ///< Reserved, bytes 3:1
	uint8_t zgorig[224]; ///< ZoneGroup Originator, bytes 227:4
	uint8_t zgname[30]; ///< ZoneGroup Name, bytes 257:228
};

/**
 * FZR Data for GAZ (Base 2.4, 8.3.2.3.8.2.1, Figure 828, page 776, PDF page 806)
 *
 * Followed, from byte 16, by as many ZoneGroup Fragment as the data says
 */
struct __attribute__((packed)) xnvme_spec_base_fzr_data_for_gaz {
	uint8_t otyp; ///< Operation Type, byte 0
	uint8_t rsvd1[3]; ///< Reserved, bytes 3:1
	uint32_t zggn; ///< ZoneGroup Generation Number, bytes 7:4
	uint32_t zgfl; ///< ZoneGroup Fragment Length, bytes 11:8
	uint32_t rsvd12; ///< Reserved, bytes 15:12
};

/**
 * FZL Data for AAZ (Base 2.4, 8.3.2.3.8.2.2, Figure 830, page 777, PDF page 807)
 */
struct __attribute__((packed)) xnvme_spec_base_fzl_data_for_aaz {
	uint8_t otyp; ///< Operation Type, byte 0
	uint8_t rsvd1[3]; ///< Reserved, bytes 3:1
	uint8_t zgorig[224]; ///< ZoneGroup Originator, bytes 227:4
	uint8_t zgname[30]; ///< ZoneGroup Name, bytes 257:228
};

/**
 * FZS Data for AAZ (Base 2.4, 8.3.2.3.8.2.2, Figure 831, page 778, PDF page 808)
 *
 * Followed, from byte 4, by as many ZoneGroup Fragment as the data says
 */
struct __attribute__((packed)) xnvme_spec_base_fzs_data_for_aaz {
	uint8_t otyp; ///< Operation Type, byte 0
	uint8_t rsvd1[3]; ///< Reserved, bytes 3:1
};

/**
 * FZL Data for RAZ (Base 2.4, 8.3.2.3.8.2.3, Figure 833, page 779, PDF page 809)
 */
struct __attribute__((packed)) xnvme_spec_base_fzl_data_for_raz {
	uint8_t otyp; ///< Operation Type, byte 0
	uint8_t rsvd1[3]; ///< Reserved, bytes 3:1
	uint8_t zgorig[224]; ///< ZoneGroup Originator, bytes 227:4
	uint8_t zgname[30]; ///< ZoneGroup Name, bytes 257:228
};

/**
 * GAZ Operation Specific Parameters for Pull Model DDC Request (Base 2.4, 8.3.2.3.8.3.1, Figure 835, page 780, PDF page 810)
 */
struct __attribute__((packed)) xnvme_spec_base_log_gaz_operation_specific_parameters_for_pull_model_ddc_request {
	uint32_t t_id; ///< Transaction ID, bytes 3:0
	uint8_t zgorig[224]; ///< ZoneGroup Originator, bytes 227:4
	uint8_t zgname[30]; ///< ZoneGroup Name, bytes 257:228
};

/**
 * FZS Data for Pull Model GAZ (Base 2.4, 8.3.2.3.8.3.1, Figure 836, page 780, PDF page 810)
 *
 * Followed, from byte 16, by as many ZoneGroup Fragment as the data says
 */
struct __attribute__((packed)) xnvme_spec_base_fzs_data_for_pull_model_gaz {
	uint8_t otyp; ///< Operation Type, byte 0
	uint8_t rsvd1[3]; ///< Reserved, bytes 3:1
	uint32_t gazos; ///< GAZ Operation Status, bytes 7:4
	uint32_t zgfl; ///< ZoneGroup Fragment Length, bytes 11:8
	uint32_t rsvd12; ///< Reserved, bytes 15:12
};

/**
 * AAZ Operation Specific Parameters for Pull Model DDC Request (Base 2.4, 8.3.2.3.8.3.2, Figure 838, page 782, PDF page 812)
 */
struct __attribute__((packed)) xnvme_spec_base_log_aaz_operation_specific_parameters_for_pull_model_ddc_request {
	uint32_t t_id; ///< Transaction ID, bytes 3:0
	uint8_t zgorig[224]; ///< ZoneGroup Originator, bytes 227:4
	uint8_t zgname[30]; ///< ZoneGroup Name, bytes 257:228
};

/**
 * FZR Data for Pull Model AAZ (Base 2.4, 8.3.2.3.8.3.2, Figure 839, page 782, PDF page 812)
 *
 * Followed, from byte 16, by as many ZoneGroup Fragment as the data says
 */
struct __attribute__((packed)) xnvme_spec_base_fzr_data_for_pull_model_aaz {
	uint8_t otyp; ///< Operation Type, byte 0
	uint8_t rsvd1[7]; ///< Reserved, bytes 7:1
	uint32_t zgfl; ///< ZoneGroup Fragment Length, bytes 11:8
	uint32_t rsvd12; ///< Reserved, bytes 15:12
};

/**
 * FZS Data for Pull Model AAZ (Base 2.4, 8.3.2.3.8.3.2, Figure 840, page 782, PDF page 812)
 */
struct __attribute__((packed)) xnvme_spec_base_fzs_data_for_pull_model_aaz {
	uint8_t otyp; ///< Operation Type, byte 0
	uint8_t rsvd1[3]; ///< Reserved, bytes 3:1
	uint32_t zaaos; ///< AAZ Operation Status, bytes 7:4
};

/**
 * RAZ Operation Specific Parameters for Pull Model DDC Request (Base 2.4, 8.3.2.3.8.3.3, Figure 842, page 783, PDF page 813)
 */
struct __attribute__((packed)) xnvme_spec_base_log_raz_operation_specific_parameters_for_pull_model_ddc_request {
	uint32_t t_id; ///< Transaction ID, bytes 3:0
	uint8_t zgorig[224]; ///< ZoneGroup Originator, bytes 227:4
	uint8_t zgname[30]; ///< ZoneGroup Name, bytes 257:228
};

/**
 * FZS Data for Pull Model RAZ (Base 2.4, 8.3.2.3.8.3.3, Figure 843, page 784, PDF page 814)
 */
struct __attribute__((packed)) xnvme_spec_base_fzs_data_for_pull_model_raz {
	uint8_t otyp; ///< Operation Type, byte 0
	uint8_t rsvd1[3]; ///< Reserved, bytes 3:1
	uint32_t razos; ///< RAZ Operation Status, bytes 7:4
};

/**
 * Log Page Request Operation Specific Parameters for Pull Model DDC Request (Base 2.4, 8.3.2.3.8.3.5, Figure 846, page 785, PDF page 815)
 */
struct __attribute__((packed)) xnvme_spec_base_log_log_page_request_operation_specific_parameters_for_pull_model_ddc_request {
	uint8_t lprlid; ///< Log Page Request Log Page Identifier, byte 0
	uint8_t lprlsp; ///< Log Page Request Log Specific Parameter, byte 1
	uint16_t rsvd2; ///< Reserved, bytes 3:2
};

/**
 * AUTH_Negotiate message format (Base 2.4, 8.3.4.4.1, Figure 853, page 795, PDF page 825)
 *
 * Followed, from byte 8, by as many Authentication Protocol Descriptor as the data says
 */
struct __attribute__((packed)) xnvme_spec_base_auth_negotiate_message_format {
	uint8_t auth_type; ///< Authentication Type, byte 0
	uint8_t auth_id; ///< Authentication Identifier, byte 1
	uint16_t rsvd2; ///< Reserved, bytes 3:2
	uint16_t t_id; ///< Transaction Identifier, bytes 5:4
	uint8_t sc_c; ///< Secure Channel Concatenation, byte 6
	uint8_t napd; ///< Number of Authentication Protocol Descriptors, byte 7
};

/**
 * AUTH_Failure1 and AUTH_Failure2 message format (Base 2.4, 8.3.4.4.2, Figure 856, page 796, PDF page 826)
 */
struct __attribute__((packed)) xnvme_spec_base_auth_failure1_and_auth_failure2_message_format {
	uint8_t auth_type; ///< Authentication Type, byte 0
	uint8_t auth_id; ///< Authentication Identifier, byte 1
	uint16_t rsvd2; ///< Reserved, bytes 3:2
	uint16_t t_id; ///< Transaction Identifier, bytes 5:4
	uint8_t rcode; ///< Reason Code, byte 6
	uint8_t rcodeex; ///< Reason Code Explanation, byte 7
};

/**
 * Authentication protocol descriptor for KX-HMAC-CHAP (Base 2.4, 8.3.4.5.2, Figure 863, page 801, PDF page 831)
 *
 * Followed, from byte 4, by as many Hash Function Identifier List as the data says
 *
 * Not members:
 * Pad: at an offset the data gives, in 'hash_function_identifier_list'
 * Key Exchange Group Identifier List: at an offset the data gives, in 'hash_function_identifier_list'
 * Pad1: at an offset the data gives, in 'hash_function_identifier_list'
 */
struct __attribute__((packed)) xnvme_spec_base_authentication_protocol_descriptor_for_kx_hmac_chap {
	uint8_t authid; ///< Authentication Protocol Identifier, byte 0
	uint8_t rsvd1; ///< Reserved, byte 1
	uint8_t halen; ///< HashIDList Length, byte 2
	uint8_t kxlen; ///< KXgIDList Length, byte 3
};

/**
 * KX-HMAC-CHAP_Challenge message format (Base 2.4, 8.3.4.5.3, Figure 874, page 806, PDF page 836)
 *
 * Followed, from byte 16, by as many Challenge Value as the data says
 *
 * Not members:
 * KX Value: at an offset the data gives, in 'challenge_value'
 */
struct __attribute__((packed)) xnvme_spec_base_kx_hmac_chap_challenge_message_format {
	uint8_t auth_type; ///< Authentication Type, byte 0
	uint8_t auth_id; ///< Authentication Identifier, byte 1
	uint16_t rsvd2; ///< Reserved, bytes 3:2
	uint16_t t_id; ///< Transaction Identifier, bytes 5:4
	uint8_t hl; ///< Hash Length, byte 6
	uint8_t rsvd7; ///< Reserved, byte 7
	uint8_t hashid; ///< Hash Identifier, byte 8
	uint8_t kxgid; ///< Key Exchange Group Identifier, byte 9
	uint16_t kxvlen; ///< KX Value Length, bytes 11:10
	uint32_t seqnum; ///< Sequence Number, bytes 15:12
};

/**
 * KX-HMAC-CHAP_Reply message format (Base 2.4, 8.3.4.5.4, Figure 875, page 807, PDF page 837)
 *
 * Followed, from byte 16, by as many Response Value as the data says
 *
 * Not members:
 * Challenge Value: at an offset the data gives, in 'response_value'
 * KX Value: at an offset the data gives, in 'response_value'
 */
struct __attribute__((packed)) xnvme_spec_base_kx_hmac_chap_reply_message_format {
	uint8_t auth_type; ///< Authentication Type, byte 0
	uint8_t auth_id; ///< Authentication Identifier, byte 1
	uint16_t rsvd2; ///< Reserved, bytes 3:2
	uint16_t t_id; ///< Transaction Identifier, bytes 5:4
	uint8_t hl; ///< Hash Length, byte 6
	uint8_t rsvd7; ///< Reserved, byte 7
	uint8_t cvalid; ///< Challenge Valid, byte 8
	uint8_t rsvd9; ///< Reserved, byte 9
	uint16_t kxvlen; ///< KX Value Length, bytes 11:10
	uint32_t seqnum; ///< Sequence Number, bytes 15:12
};

/**
 * KX-HMAC-CHAP_Success1 message format (Base 2.4, 8.3.4.5.5, Figure 876, page 809, PDF page 839)
 *
 * Followed, from byte 16, by as many Response Value as the data says
 */
struct __attribute__((packed)) xnvme_spec_base_kx_hmac_chap_success1_message_format {
	uint8_t auth_type; ///< Authentication Type, byte 0
	uint8_t auth_id; ///< Authentication Identifier, byte 1
	uint16_t rsvd2; ///< Reserved, bytes 3:2
	uint16_t t_id; ///< Transaction Identifier, bytes 5:4
	uint8_t hl; ///< Hash Length, byte 6
	uint8_t rsvd7; ///< Reserved, byte 7
	uint8_t rvalid; ///< Response Valid, byte 8
	uint8_t rsvd9[7]; ///< Reserved, bytes 15:9
};

/**
 * KX-HMAC-CHAP_Success2 message format (Base 2.4, 8.3.4.5.6, Figure 877, page 810, PDF page 840)
 */
struct __attribute__((packed)) xnvme_spec_base_kx_hmac_chap_success2_message_format {
	uint8_t auth_type; ///< Authentication Type, byte 0
	uint8_t auth_id; ///< Authentication Identifier, byte 1
	uint16_t rsvd2; ///< Reserved, bytes 3:2
	uint16_t t_id; ///< Transaction Identifier, bytes 5:4
	uint8_t rsvd6[10]; ///< Reserved, bytes 15:6
};

/**
 * KX-HMAC-CHAP_Access-Request PDU format (Base 2.4, 8.3.4.6.3, Figure 887, page 818, PDF page 848)
 *
 * Followed, from byte 28, by as many Augmented Challenge Value as the data says
 *
 * Not members:
 * Response Value: at an offset the data gives, in 'augmented_challenge_value'
 * NQN of Responder: at an offset the data gives, in 'augmented_challenge_value'
 */
struct __attribute__((packed)) xnvme_spec_base_kx_hmac_chap_access_request_pdu_format {
	uint8_t pdu_type; ///< PDU Type, byte 0
	uint8_t flags; ///< Flags, byte 1
	uint8_t hlen; ///< Header Length, byte 2
	uint8_t pdo; ///< PDU Data Offset, byte 3
	uint32_t plen; ///< PDU Length, bytes 7:4
	uint64_t id; ///< Identifier, bytes 15:8
	uint8_t hl; ///< Hash Length, byte 16
	uint8_t hashid; ///< Hash Identifier, byte 17
	uint16_t t_id; ///< Transaction Identifier, bytes 19:18
	uint8_t sc_c; ///< Secure Channel Concatenation, byte 20
	uint8_t respr; ///< Responder's Role, byte 21
	uint8_t nqnrlen; ///< NQN Responder Length, byte 22
	uint8_t rsvd23; ///< Reserved, byte 23
	uint32_t seqn; ///< Sequence Number, bytes 27:24
};

/**
 * KX-HMAC-CHAP_Access-Result PDU format (Base 2.4, 8.3.4.6.3, Figure 888, page 820, PDF page 850)
 */
struct __attribute__((packed)) xnvme_spec_base_kx_hmac_chap_access_result_pdu_format {
	uint8_t pdu_type; ///< PDU Type, byte 0
	uint8_t flags; ///< Flags, byte 1
	uint8_t hlen; ///< Header Length, byte 2
	uint8_t pdo; ///< PDU Data Offset, byte 3
	uint32_t plen; ///< PDU Length, bytes 7:4
	uint64_t id; ///< Identifier, bytes 15:8
	uint8_t authres; ///< Authentication Verification Result, byte 16
	uint8_t rcode; ///< Reason Code, byte 17
	uint16_t rsvd18; ///< Reserved, bytes 19:18
};

/**
 * The values of Authentication Verification Result (Base 2.4, 8.3.4.6.3, Figure 888, page 820, PDF page 850)
 */
enum xnvme_spec_base_kx_hmac_chap_access_result_pdu_format_authres {
	XNVME_SPEC_BASE_KX_HMAC_CHAP_ACCESS_RESULT_PDU_FORMAT_AUTHRES_AUTHENTICATION_VERIFICATION_SUCCESSFUL = 0x1, ///< Authentication Verification Successful (Base 2.4, 8.3.4.6.3, Figure 888, page 820, PDF page 850)
	XNVME_SPEC_BASE_KX_HMAC_CHAP_ACCESS_RESULT_PDU_FORMAT_AUTHRES_AUTHENTICATION_VERIFICATION_FAILED = 0x2, ///< Authentication Verification Failed (Base 2.4, 8.3.4.6.3, Figure 888, page 820, PDF page 850)
};

/**
 * Completion Queue Entry, of the layouts of its dwords (Base 2.4, 4.2.1, Figure 98, page 144, PDF page 170; Base 2.4, 4.2.1, Figure 99, page 145, PDF page 171)
 */
struct __attribute__((packed)) xnvme_spec_base_completion_queue_entry {
	uint32_t dw0; ///< Dword 0, of no layout in the data model, bytes 3:0
	uint32_t dw1; ///< Dword 1, of no layout in the data model, bytes 7:4
	uint16_t sqhd; ///< SQ Head Pointer, bytes 9:8
	uint16_t sqid; ///< SQ Identifier, bytes 11:10
	uint16_t cid; ///< Command Identifier, bytes 13:12
	uint16_t status; ///< Status and Phase Tag, bytes 15:14, see XNVME_SPEC_BASE_COMPLETION_QUEUE_ENTRY_STATUS_*
};

/** Do Not Retry (Base 2.4, 4.2.3, Figure 101, page 145, PDF page 171); bit 15 of 'status' */
#define XNVME_SPEC_BASE_COMPLETION_QUEUE_ENTRY_STATUS_DNR 0x8000
/** More (Base 2.4, 4.2.3, Figure 101, page 145, PDF page 171); bit 14 of 'status' */
#define XNVME_SPEC_BASE_COMPLETION_QUEUE_ENTRY_STATUS_M 0x4000
/** Command Retry Delay (Base 2.4, 4.2.3, Figure 101, page 145, PDF page 171); bits 13:12 of 'status' */
#define XNVME_SPEC_BASE_COMPLETION_QUEUE_ENTRY_STATUS_CRD 0x3000
/** Status Code Type (Base 2.4, 4.2.3, Figure 101, page 145, PDF page 171); bits 11:9 of 'status' */
#define XNVME_SPEC_BASE_COMPLETION_QUEUE_ENTRY_STATUS_SCT 0xe00
/** Status Code (Base 2.4, 4.2.3, Figure 101, page 145, PDF page 171); bits 8:1 of 'status' */
#define XNVME_SPEC_BASE_COMPLETION_QUEUE_ENTRY_STATUS_SC 0x1fe
/** Phase Tag (Base 2.4, 4.2.1, Figure 99, page 145, PDF page 171); bit 0 of 'status' */
#define XNVME_SPEC_BASE_COMPLETION_QUEUE_ENTRY_STATUS_P 0x1

/**
 * The registers of the base specification, at their offsets
 */
struct __attribute__((packed)) xnvme_spec_base_registers {
	uint64_t cap; ///< CAP - Controller Capabilities (Base 2.4, 3.1.4.1, Figure 36, page 55, PDF page 81), at 0h, see XNVME_SPEC_BASE_REGISTERS_CAP_*
	uint32_t vs; ///< VS - Version (Base 2.4, 3.1.4.2, Figure 37, page 58, PDF page 84), at 8h, see XNVME_SPEC_BASE_REGISTERS_VS_*
	uint32_t intms; ///< INTMS - Interrupt Mask Set (Base 2.4, 3.1.4.3, Figure 39, page 59, PDF page 85), at Ch, see XNVME_SPEC_BASE_REGISTERS_INTMS_*
	uint32_t intmc; ///< INTMC - Interrupt Mask Clear (Base 2.4, 3.1.4.4, Figure 40, page 59, PDF page 85), at 10h, see XNVME_SPEC_BASE_REGISTERS_INTMC_*
	uint32_t cc; ///< CC - Controller Configuration (Base 2.4, 3.1.4.5, Figure 41, page 60, PDF page 86), at 14h, see XNVME_SPEC_BASE_REGISTERS_CC_*
	uint8_t rsvd18[4]; ///< Reserved, 18h to 1Bh
	uint32_t csts; ///< CSTS - Controller Status (Base 2.4, 3.1.4.6, Figure 42, page 63, PDF page 89), at 1Ch, see XNVME_SPEC_BASE_REGISTERS_CSTS_*
	uint32_t nssr; ///< NSSR - NVM Subsystem Reset (Base 2.4, 3.1.4.7, Figure 43, page 66, PDF page 92), at 20h, see XNVME_SPEC_BASE_REGISTERS_NSSR_*
	uint32_t aqa; ///< AQA - Admin Queue Attributes (Base 2.4, 3.1.4.8, Figure 44, page 66, PDF page 92), at 24h, see XNVME_SPEC_BASE_REGISTERS_AQA_*
	uint64_t asq; ///< ASQ - Admin Submission Queue Base Address (Base 2.4, 3.1.4.9, Figure 45, page 66, PDF page 92), at 28h, see XNVME_SPEC_BASE_REGISTERS_ASQ_*
	uint64_t acq; ///< ACQ - Admin Completion Queue Base Address (Base 2.4, 3.1.4.10, Figure 46, page 67, PDF page 93), at 30h, see XNVME_SPEC_BASE_REGISTERS_ACQ_*
	uint32_t cmbloc; ///< CMBLOC - Controller Memory Buffer Location (Base 2.4, 3.1.4.11, Figure 47, page 67, PDF page 93), at 38h, see XNVME_SPEC_BASE_REGISTERS_CMBLOC_*
	uint32_t cmbsz; ///< CMBSZ - Controller Memory Buffer Size (Base 2.4, 3.1.4.12, Figure 48, page 68, PDF page 94), at 3Ch, see XNVME_SPEC_BASE_REGISTERS_CMBSZ_*
	uint32_t bpinfo; ///< BPINFO - Boot Partition Information (Base 2.4, 3.1.4.13, Figure 49, page 69, PDF page 95), at 40h, see XNVME_SPEC_BASE_REGISTERS_BPINFO_*
	uint32_t bprsel; ///< BPRSEL - Boot Partition Read Select (Base 2.4, 3.1.4.14, Figure 50, page 69, PDF page 95), at 44h, see XNVME_SPEC_BASE_REGISTERS_BPRSEL_*
	uint64_t bpmbl; ///< BPMBL - Boot Partition Memory Buffer Location (Base 2.4, 3.1.4.15, Figure 51, page 70, PDF page 96), at 48h, see XNVME_SPEC_BASE_REGISTERS_BPMBL_*
	uint64_t cmbmsc; ///< CMBMSC - Controller Memory Buffer Memory Space Control (Base 2.4, 3.1.4.16, Figure 52, page 70, PDF page 96), at 50h, see XNVME_SPEC_BASE_REGISTERS_CMBMSC_*
	uint32_t cmbsts; ///< CMBSTS - Controller Memory Buffer Status (Base 2.4, 3.1.4.17, Figure 53, page 71, PDF page 97), at 58h, see XNVME_SPEC_BASE_REGISTERS_CMBSTS_*
	uint32_t cmbebs; ///< CMBEBS - Controller Memory Buffer Elasticity Buffer Size (Base 2.4, 3.1.4.18, Figure 54, page 71, PDF page 97), at 5Ch, see XNVME_SPEC_BASE_REGISTERS_CMBEBS_*
	uint32_t cmbswtp; ///< CMBSWTP - Controller Memory Buffer Sustained Write Throughput (Base 2.4, 3.1.4.19, Figure 55, page 72, PDF page 98), at 60h, see XNVME_SPEC_BASE_REGISTERS_CMBSWTP_*
	uint32_t nssd; ///< NSSD - NVM Subsystem Shutdown (Base 2.4, 3.1.4.20, Figure 56, page 72, PDF page 98), at 64h, see XNVME_SPEC_BASE_REGISTERS_NSSD_*
	uint32_t crto; ///< CRTO - Controller Ready Timeouts (Base 2.4, 3.1.4.21, Figure 57, page 73, PDF page 99), at 68h, see XNVME_SPEC_BASE_REGISTERS_CRTO_*
	uint8_t rsvd6c[3476]; ///< Reserved, 6Ch to DFFh
	uint32_t pmrcap; ///< PMRCAP - Persistent Memory Region Capabilities (Base 2.4, 3.1.4.22, Figure 58, page 73, PDF page 99), at E00h, see XNVME_SPEC_BASE_REGISTERS_PMRCAP_*
	uint32_t pmrctl; ///< PMRCTL - Persistent Memory Region Control (Base 2.4, 3.1.4.23, Figure 59, page 74, PDF page 100), at E04h, see XNVME_SPEC_BASE_REGISTERS_PMRCTL_*
	uint32_t pmrsts; ///< PMRSTS - Persistent Memory Region Status (Base 2.4, 3.1.4.24, Figure 60, page 75, PDF page 101), at E08h, see XNVME_SPEC_BASE_REGISTERS_PMRSTS_*
	uint32_t pmrebs; ///< PMREBS - Persistent Memory Region Elasticity Buffer Size (Base 2.4, 3.1.4.25, Figure 61, page 76, PDF page 102), at E0Ch, see XNVME_SPEC_BASE_REGISTERS_PMREBS_*
	uint32_t pmrswtp; ///< PMRSWTP - Persistent Memory Region Sustained Write Throughput (Base 2.4, 3.1.4.26, Figure 62, page 76, PDF page 102), at E10h, see XNVME_SPEC_BASE_REGISTERS_PMRSWTP_*
	uint32_t pmrmscl; ///< PMRMSCL - Persistent Memory Region Memory Space Control Lower (Base 2.4, 3.1.4.27, Figure 63, page 77, PDF page 103), at E14h, see XNVME_SPEC_BASE_REGISTERS_PMRMSCL_*
	uint32_t pmrmscu; ///< PMRMSCU - Persistent Memory Region Memory Space Control Upper (Base 2.4, 3.1.4.28, Figure 64, page 77, PDF page 103), at E18h, see XNVME_SPEC_BASE_REGISTERS_PMRMSCU_*
};

/** NVM Subsystem Shutdown Enhancements Supported (Base 2.4, 3.1.4.1, Figure 36, page 55, PDF page 81); bit 61 of 'cap' */
#define XNVME_SPEC_BASE_REGISTERS_CAP_NSSES 0x2000000000000000
/** Controller Ready Modes Supported (Base 2.4, 3.1.4.1, Figure 36, page 55, PDF page 81); bits 60:59 of 'cap' */
#define XNVME_SPEC_BASE_REGISTERS_CAP_CRMS 0x1800000000000000
/** NVM Subsystem Shutdown Supported (Base 2.4, 3.1.4.1, Figure 36, page 55, PDF page 81); bit 58 of 'cap' */
#define XNVME_SPEC_BASE_REGISTERS_CAP_NSSS 0x400000000000000
/** Controller Memory Buffer Supported (Base 2.4, 3.1.4.1, Figure 36, page 55, PDF page 81); bit 57 of 'cap' */
#define XNVME_SPEC_BASE_REGISTERS_CAP_CMBS 0x200000000000000
/** Persistent Memory Region Supported (Base 2.4, 3.1.4.1, Figure 36, page 55, PDF page 81); bit 56 of 'cap' */
#define XNVME_SPEC_BASE_REGISTERS_CAP_PMRS 0x100000000000000
/** Memory Page Size Maximum (Base 2.4, 3.1.4.1, Figure 36, page 55, PDF page 81); bits 55:52 of 'cap' */
#define XNVME_SPEC_BASE_REGISTERS_CAP_MPSMAX 0xf0000000000000
/** Memory Page Size Minimum (Base 2.4, 3.1.4.1, Figure 36, page 55, PDF page 81); bits 51:48 of 'cap' */
#define XNVME_SPEC_BASE_REGISTERS_CAP_MPSMIN 0xf000000000000
/** Controller Power Scope (Base 2.4, 3.1.4.1, Figure 36, page 55, PDF page 81); bits 47:46 of 'cap' */
#define XNVME_SPEC_BASE_REGISTERS_CAP_CPS 0xc00000000000
/** Boot Partition Support (Base 2.4, 3.1.4.1, Figure 36, page 55, PDF page 81); bit 45 of 'cap' */
#define XNVME_SPEC_BASE_REGISTERS_CAP_BPS 0x200000000000
/** Command Sets Supported (Base 2.4, 3.1.4.1, Figure 36, page 55, PDF page 81); bits 44:37 of 'cap' */
#define XNVME_SPEC_BASE_REGISTERS_CAP_CSS 0x1fe000000000
/** NVM Subsystem Reset Supported (Base 2.4, 3.1.4.1, Figure 36, page 55, PDF page 81); bit 36 of 'cap' */
#define XNVME_SPEC_BASE_REGISTERS_CAP_NSSRS 0x1000000000
/** Doorbell Stride (Base 2.4, 3.1.4.1, Figure 36, page 55, PDF page 81); bits 35:32 of 'cap' */
#define XNVME_SPEC_BASE_REGISTERS_CAP_DSTRD 0xf00000000
/** Timeout (Base 2.4, 3.1.4.1, Figure 36, page 55, PDF page 81); bits 31:24 of 'cap' */
#define XNVME_SPEC_BASE_REGISTERS_CAP_TO 0xff000000
/** Arbitration Mechanism Supported (Base 2.4, 3.1.4.1, Figure 36, page 55, PDF page 81); bits 18:17 of 'cap' */
#define XNVME_SPEC_BASE_REGISTERS_CAP_AMS 0x60000
/** Contiguous Queues Required (Base 2.4, 3.1.4.1, Figure 36, page 55, PDF page 81); bit 16 of 'cap' */
#define XNVME_SPEC_BASE_REGISTERS_CAP_CQR 0x10000
/** Maximum Queue Entries Supported (Base 2.4, 3.1.4.1, Figure 36, page 55, PDF page 81); bits 15:0 of 'cap' */
#define XNVME_SPEC_BASE_REGISTERS_CAP_MQES 0xffff
/** Major Version (Base 2.4, 3.1.4.2, Figure 37, page 58, PDF page 84); bits 31:16 of 'vs' */
#define XNVME_SPEC_BASE_REGISTERS_VS_MJR 0xffff0000
/** Minor Version (Base 2.4, 3.1.4.2, Figure 37, page 58, PDF page 84); bits 15:8 of 'vs' */
#define XNVME_SPEC_BASE_REGISTERS_VS_MNR 0xff00
/** Tertiary Version (Base 2.4, 3.1.4.2, Figure 37, page 58, PDF page 84); bits 7:0 of 'vs' */
#define XNVME_SPEC_BASE_REGISTERS_VS_TER 0xff
/** Interrupt Vector Mask Set (Base 2.4, 3.1.4.3, Figure 39, page 59, PDF page 85); bits 31:0 of 'intms' */
#define XNVME_SPEC_BASE_REGISTERS_INTMS_IVMS 0xffffffff
/** Interrupt Vector Mask Clear (Base 2.4, 3.1.4.4, Figure 40, page 59, PDF page 85); bits 31:0 of 'intmc' */
#define XNVME_SPEC_BASE_REGISTERS_INTMC_IVMC 0xffffffff
/** Controller Ready Independent of Media Enable (Base 2.4, 3.1.4.5, Figure 41, page 60, PDF page 86); bit 24 of 'cc' */
#define XNVME_SPEC_BASE_REGISTERS_CC_CRIME 0x1000000
/** I/O Completion Queue Entry Size (Base 2.4, 3.1.4.5, Figure 41, page 60, PDF page 86); bits 23:20 of 'cc' */
#define XNVME_SPEC_BASE_REGISTERS_CC_IOCQES 0xf00000
/** I/O Submission Queue Entry Size (Base 2.4, 3.1.4.5, Figure 41, page 60, PDF page 86); bits 19:16 of 'cc' */
#define XNVME_SPEC_BASE_REGISTERS_CC_IOSQES 0xf0000
/** Shutdown Notification (Base 2.4, 3.1.4.5, Figure 41, page 60, PDF page 86); bits 15:14 of 'cc' */
#define XNVME_SPEC_BASE_REGISTERS_CC_SHN 0xc000
/** Arbitration Mechanism Selected (Base 2.4, 3.1.4.5, Figure 41, page 60, PDF page 86); bits 13:11 of 'cc' */
#define XNVME_SPEC_BASE_REGISTERS_CC_AMS 0x3800
/** Memory Page Size (Base 2.4, 3.1.4.5, Figure 41, page 60, PDF page 86); bits 10:7 of 'cc' */
#define XNVME_SPEC_BASE_REGISTERS_CC_MPS 0x780
/** I/O Command Set Selected (Base 2.4, 3.1.4.5, Figure 41, page 60, PDF page 86); bits 6:4 of 'cc' */
#define XNVME_SPEC_BASE_REGISTERS_CC_CSS 0x70
/** Enable (Base 2.4, 3.1.4.5, Figure 41, page 60, PDF page 86); bit 0 of 'cc' */
#define XNVME_SPEC_BASE_REGISTERS_CC_EN 0x1
/** Shutdown Type (Base 2.4, 3.1.4.6, Figure 42, page 63, PDF page 89); bit 6 of 'csts' */
#define XNVME_SPEC_BASE_REGISTERS_CSTS_ST 0x40
/** Processing Paused (Base 2.4, 3.1.4.6, Figure 42, page 63, PDF page 89); bit 5 of 'csts' */
#define XNVME_SPEC_BASE_REGISTERS_CSTS_PP 0x20
/** NVM Subsystem Reset Occurred (Base 2.4, 3.1.4.6, Figure 42, page 63, PDF page 89); bit 4 of 'csts' */
#define XNVME_SPEC_BASE_REGISTERS_CSTS_NSSRO 0x10
/** Shutdown Status (Base 2.4, 3.1.4.6, Figure 42, page 63, PDF page 89); bits 3:2 of 'csts' */
#define XNVME_SPEC_BASE_REGISTERS_CSTS_SHST 0xc
/** Controller Fatal Status (Base 2.4, 3.1.4.6, Figure 42, page 63, PDF page 89); bit 1 of 'csts' */
#define XNVME_SPEC_BASE_REGISTERS_CSTS_CFS 0x2
/** Ready (Base 2.4, 3.1.4.6, Figure 42, page 63, PDF page 89); bit 0 of 'csts' */
#define XNVME_SPEC_BASE_REGISTERS_CSTS_RDY 0x1
/** NVM Subsystem Reset Control (Base 2.4, 3.1.4.7, Figure 43, page 66, PDF page 92); bits 31:0 of 'nssr' */
#define XNVME_SPEC_BASE_REGISTERS_NSSR_NSSRC 0xffffffff
/** Admin Completion Queue Size (Base 2.4, 3.1.4.8, Figure 44, page 66, PDF page 92); bits 27:16 of 'aqa' */
#define XNVME_SPEC_BASE_REGISTERS_AQA_ACQS 0xfff0000
/** Admin Submission Queue Size (Base 2.4, 3.1.4.8, Figure 44, page 66, PDF page 92); bits 11:0 of 'aqa' */
#define XNVME_SPEC_BASE_REGISTERS_AQA_ASQS 0xfff
/** Admin Submission Queue Base (Base 2.4, 3.1.4.9, Figure 45, page 66, PDF page 92); bits 63:12 of 'asq' */
#define XNVME_SPEC_BASE_REGISTERS_ASQ_ASQB 0xfffffffffffff000
/** Admin Completion Queue Base (Base 2.4, 3.1.4.10, Figure 46, page 67, PDF page 93); bits 63:12 of 'acq' */
#define XNVME_SPEC_BASE_REGISTERS_ACQ_ACQB 0xfffffffffffff000
/** Offset (Base 2.4, 3.1.4.11, Figure 47, page 67, PDF page 93); bits 31:12 of 'cmbloc' */
#define XNVME_SPEC_BASE_REGISTERS_CMBLOC_OFST 0xfffff000
/** CMB Queue Dword Alignment (Base 2.4, 3.1.4.11, Figure 47, page 67, PDF page 93); bit 8 of 'cmbloc' */
#define XNVME_SPEC_BASE_REGISTERS_CMBLOC_CQDA 0x100
/** CMB Data Metadata Mixed Memory Support (Base 2.4, 3.1.4.11, Figure 47, page 67, PDF page 93); bit 7 of 'cmbloc' */
#define XNVME_SPEC_BASE_REGISTERS_CMBLOC_CDMMMS 0x80
/** CMB Data Pointer and Command Independent Locations Support (Base 2.4, 3.1.4.11, Figure 47, page 67, PDF page 93); bit 6 of 'cmbloc' */
#define XNVME_SPEC_BASE_REGISTERS_CMBLOC_CDPCILS 0x40
/** CMB Data Pointer Mixed Locations Support (Base 2.4, 3.1.4.11, Figure 47, page 67, PDF page 93); bit 5 of 'cmbloc' */
#define XNVME_SPEC_BASE_REGISTERS_CMBLOC_CDPMLS 0x20
/** CMB Queue Physically Discontiguous Support (Base 2.4, 3.1.4.11, Figure 47, page 67, PDF page 93); bit 4 of 'cmbloc' */
#define XNVME_SPEC_BASE_REGISTERS_CMBLOC_CQPDS 0x10
/** CMB Queue Mixed Memory Support (Base 2.4, 3.1.4.11, Figure 47, page 67, PDF page 93); bit 3 of 'cmbloc' */
#define XNVME_SPEC_BASE_REGISTERS_CMBLOC_CQMMS 0x8
/** Base Indicator Register (Base 2.4, 3.1.4.11, Figure 47, page 67, PDF page 93); bits 2:0 of 'cmbloc' */
#define XNVME_SPEC_BASE_REGISTERS_CMBLOC_BIR 0x7
/** Size (Base 2.4, 3.1.4.12, Figure 48, page 68, PDF page 94); bits 31:12 of 'cmbsz' */
#define XNVME_SPEC_BASE_REGISTERS_CMBSZ_SZ 0xfffff000
/** Size Units (Base 2.4, 3.1.4.12, Figure 48, page 68, PDF page 94); bits 11:8 of 'cmbsz' */
#define XNVME_SPEC_BASE_REGISTERS_CMBSZ_SZU 0xf00
/** Write Data Support (Base 2.4, 3.1.4.12, Figure 48, page 68, PDF page 94); bit 4 of 'cmbsz' */
#define XNVME_SPEC_BASE_REGISTERS_CMBSZ_WDS 0x10
/** Read Data Support (Base 2.4, 3.1.4.12, Figure 48, page 68, PDF page 94); bit 3 of 'cmbsz' */
#define XNVME_SPEC_BASE_REGISTERS_CMBSZ_RDS 0x8
/** PRP SGL List Support (Base 2.4, 3.1.4.12, Figure 48, page 68, PDF page 94); bit 2 of 'cmbsz' */
#define XNVME_SPEC_BASE_REGISTERS_CMBSZ_LISTS 0x4
/** Completion Queue Support (Base 2.4, 3.1.4.12, Figure 48, page 68, PDF page 94); bit 1 of 'cmbsz' */
#define XNVME_SPEC_BASE_REGISTERS_CMBSZ_CQS 0x2
/** Submission Queue Support (Base 2.4, 3.1.4.12, Figure 48, page 68, PDF page 94); bit 0 of 'cmbsz' */
#define XNVME_SPEC_BASE_REGISTERS_CMBSZ_SQS 0x1
/** Active Boot Partition ID (Base 2.4, 3.1.4.13, Figure 49, page 69, PDF page 95); bit 31 of 'bpinfo' */
#define XNVME_SPEC_BASE_REGISTERS_BPINFO_ABPID 0x80000000
/** Boot Read Status (Base 2.4, 3.1.4.13, Figure 49, page 69, PDF page 95); bits 25:24 of 'bpinfo' */
#define XNVME_SPEC_BASE_REGISTERS_BPINFO_BRS 0x3000000
/** Boot Partition Size (Base 2.4, 3.1.4.13, Figure 49, page 69, PDF page 95); bits 14:0 of 'bpinfo' */
#define XNVME_SPEC_BASE_REGISTERS_BPINFO_BPSZ 0x7fff
/** Boot Partition Identifier (Base 2.4, 3.1.4.14, Figure 50, page 69, PDF page 95); bit 31 of 'bprsel' */
#define XNVME_SPEC_BASE_REGISTERS_BPRSEL_BPID 0x80000000
/** Boot Partition Read Offset (Base 2.4, 3.1.4.14, Figure 50, page 69, PDF page 95); bits 29:10 of 'bprsel' */
#define XNVME_SPEC_BASE_REGISTERS_BPRSEL_BPROF 0x3ffffc00
/** Boot Partition Read Size (Base 2.4, 3.1.4.14, Figure 50, page 69, PDF page 95); bits 9:0 of 'bprsel' */
#define XNVME_SPEC_BASE_REGISTERS_BPRSEL_BPRSZ 0x3ff
/** Boot Partition Memory Buffer Base Address (Base 2.4, 3.1.4.15, Figure 51, page 70, PDF page 96); bits 63:12 of 'bpmbl' */
#define XNVME_SPEC_BASE_REGISTERS_BPMBL_BMBBA 0xfffffffffffff000
/** Controller Base Address (Base 2.4, 3.1.4.16, Figure 52, page 70, PDF page 96); bits 63:12 of 'cmbmsc' */
#define XNVME_SPEC_BASE_REGISTERS_CMBMSC_CBA 0xfffffffffffff000
/** Controller Memory Space Enable (Base 2.4, 3.1.4.16, Figure 52, page 70, PDF page 96); bit 1 of 'cmbmsc' */
#define XNVME_SPEC_BASE_REGISTERS_CMBMSC_CMSE 0x2
/** Capabilities Registers Enabled (Base 2.4, 3.1.4.16, Figure 52, page 70, PDF page 96); bit 0 of 'cmbmsc' */
#define XNVME_SPEC_BASE_REGISTERS_CMBMSC_CRE 0x1
/** Controller Base Address Invalid (Base 2.4, 3.1.4.17, Figure 53, page 71, PDF page 97); bit 0 of 'cmbsts' */
#define XNVME_SPEC_BASE_REGISTERS_CMBSTS_CBAI 0x1
/** CMB Elasticity Buffer Size Base (Base 2.4, 3.1.4.18, Figure 54, page 71, PDF page 97); bits 31:8 of 'cmbebs' */
#define XNVME_SPEC_BASE_REGISTERS_CMBEBS_CMBWBZ 0xffffff00
/** CMB Read Bypass Behavior (Base 2.4, 3.1.4.18, Figure 54, page 71, PDF page 97); bit 4 of 'cmbebs' */
#define XNVME_SPEC_BASE_REGISTERS_CMBEBS_CMBRBB 0x10
/** CMB Elasticity Buffer Size Units (Base 2.4, 3.1.4.18, Figure 54, page 71, PDF page 97); bits 3:0 of 'cmbebs' */
#define XNVME_SPEC_BASE_REGISTERS_CMBEBS_CMBSZU 0xf
/** CMB Sustained Write Throughput (Base 2.4, 3.1.4.19, Figure 55, page 72, PDF page 98); bits 31:8 of 'cmbswtp' */
#define XNVME_SPEC_BASE_REGISTERS_CMBSWTP_CMBSWTV 0xffffff00
/** CMB Sustained Write Throughput Units (Base 2.4, 3.1.4.19, Figure 55, page 72, PDF page 98); bits 3:0 of 'cmbswtp' */
#define XNVME_SPEC_BASE_REGISTERS_CMBSWTP_CMBSWTU 0xf
/** NVM Subsystem Shutdown Control (Base 2.4, 3.1.4.20, Figure 56, page 72, PDF page 98); bits 31:0 of 'nssd' */
#define XNVME_SPEC_BASE_REGISTERS_NSSD_NSSC 0xffffffff
/** Controller Ready Independent of Media Timeout (Base 2.4, 3.1.4.21, Figure 57, page 73, PDF page 99); bits 31:16 of 'crto' */
#define XNVME_SPEC_BASE_REGISTERS_CRTO_CRIMT 0xffff0000
/** Controller Ready With Media Timeout (Base 2.4, 3.1.4.21, Figure 57, page 73, PDF page 99); bits 15:0 of 'crto' */
#define XNVME_SPEC_BASE_REGISTERS_CRTO_CRWMT 0xffff
/** Controller Memory Space Supported (Base 2.4, 3.1.4.22, Figure 58, page 73, PDF page 99); bit 24 of 'pmrcap' */
#define XNVME_SPEC_BASE_REGISTERS_PMRCAP_CMSS 0x1000000
/** Persistent Memory Region Timeout (Base 2.4, 3.1.4.22, Figure 58, page 73, PDF page 99); bits 23:16 of 'pmrcap' */
#define XNVME_SPEC_BASE_REGISTERS_PMRCAP_PMRTO 0xff0000
/** Persistent Memory Region Write Barrier Mechanisms (Base 2.4, 3.1.4.22, Figure 58, page 73, PDF page 99); bits 13:10 of 'pmrcap' */
#define XNVME_SPEC_BASE_REGISTERS_PMRCAP_PMRWBM 0x3c00
/** Persistent Memory Region Time Units (Base 2.4, 3.1.4.22, Figure 58, page 73, PDF page 99); bits 9:8 of 'pmrcap' */
#define XNVME_SPEC_BASE_REGISTERS_PMRCAP_PMRTU 0x300
/** Base Indicator Register (Base 2.4, 3.1.4.22, Figure 58, page 73, PDF page 99); bits 7:5 of 'pmrcap' */
#define XNVME_SPEC_BASE_REGISTERS_PMRCAP_BIR 0xe0
/** Write Data Support (Base 2.4, 3.1.4.22, Figure 58, page 73, PDF page 99); bit 4 of 'pmrcap' */
#define XNVME_SPEC_BASE_REGISTERS_PMRCAP_WDS 0x10
/** Read Data Support (Base 2.4, 3.1.4.22, Figure 58, page 73, PDF page 99); bit 3 of 'pmrcap' */
#define XNVME_SPEC_BASE_REGISTERS_PMRCAP_RDS 0x8
/** Enable (Base 2.4, 3.1.4.23, Figure 59, page 74, PDF page 100); bit 0 of 'pmrctl' */
#define XNVME_SPEC_BASE_REGISTERS_PMRCTL_EN 0x1
/** Controller Base Address Invalid (Base 2.4, 3.1.4.24, Figure 60, page 75, PDF page 101); bit 12 of 'pmrsts' */
#define XNVME_SPEC_BASE_REGISTERS_PMRSTS_CBAI 0x1000
/** Health Status (Base 2.4, 3.1.4.24, Figure 60, page 75, PDF page 101); bits 11:9 of 'pmrsts' */
#define XNVME_SPEC_BASE_REGISTERS_PMRSTS_HSTS 0xe00
/** Not Ready (Base 2.4, 3.1.4.24, Figure 60, page 75, PDF page 101); bit 8 of 'pmrsts' */
#define XNVME_SPEC_BASE_REGISTERS_PMRSTS_NRDY 0x100
/** Error (Base 2.4, 3.1.4.24, Figure 60, page 75, PDF page 101); bits 7:0 of 'pmrsts' */
#define XNVME_SPEC_BASE_REGISTERS_PMRSTS_ERR 0xff
/** PMR Elasticity Buffer Size Base (Base 2.4, 3.1.4.25, Figure 61, page 76, PDF page 102); bits 31:8 of 'pmrebs' */
#define XNVME_SPEC_BASE_REGISTERS_PMREBS_PMRWBZ 0xffffff00
/** PMR Read Bypass Behavior (Base 2.4, 3.1.4.25, Figure 61, page 76, PDF page 102); bit 4 of 'pmrebs' */
#define XNVME_SPEC_BASE_REGISTERS_PMREBS_PMRRBB 0x10
/** PMR Elasticity Buffer Size Units (Base 2.4, 3.1.4.25, Figure 61, page 76, PDF page 102); bits 3:0 of 'pmrebs' */
#define XNVME_SPEC_BASE_REGISTERS_PMREBS_PMRSZU 0xf
/** PMR Sustained Write Throughput (Base 2.4, 3.1.4.26, Figure 62, page 76, PDF page 102); bits 31:8 of 'pmrswtp' */
#define XNVME_SPEC_BASE_REGISTERS_PMRSWTP_PMRSWTV 0xffffff00
/** PMR Sustained Write Throughput Units (Base 2.4, 3.1.4.26, Figure 62, page 76, PDF page 102); bits 3:0 of 'pmrswtp' */
#define XNVME_SPEC_BASE_REGISTERS_PMRSWTP_PMRSWTU 0xf
/** Controller Base Address (Base 2.4, 3.1.4.27, Figure 63, page 77, PDF page 103); bits 31:12 of 'pmrmscl' */
#define XNVME_SPEC_BASE_REGISTERS_PMRMSCL_CBA 0xfffff000
/** Controller Memory Space Enable (Base 2.4, 3.1.4.27, Figure 63, page 77, PDF page 103); bit 1 of 'pmrmscl' */
#define XNVME_SPEC_BASE_REGISTERS_PMRMSCL_CMSE 0x2
/** Controller Base Address (Base 2.4, 3.1.4.28, Figure 64, page 77, PDF page 103); bits 31:0 of 'pmrmscu' */
#define XNVME_SPEC_BASE_REGISTERS_PMRMSCU_CBA 0xffffffff

/**
 * The values of Controller Power Scope of CAP - Controller Capabilities
 */
enum xnvme_spec_base_registers_cap_cps {
	XNVME_SPEC_BASE_REGISTERS_CAP_CPS_NOT_REPORTED = 0x0, ///< Not Reported (Base 2.4, 3.1.4.1, Figure 36, page 55, PDF page 81)
	XNVME_SPEC_BASE_REGISTERS_CAP_CPS_CONTROLLER_SCOPE = 0x1, ///< Controller scope (Base 2.4, 3.1.4.1, Figure 36, page 55, PDF page 81)
	XNVME_SPEC_BASE_REGISTERS_CAP_CPS_DOMAIN_SCOPE = 0x2, ///< Domain scope (Base 2.4, 3.1.4.1, Figure 36, page 55, PDF page 81)
	XNVME_SPEC_BASE_REGISTERS_CAP_CPS_NVM_SUBSYSTEM_SCOPE = 0x3, ///< NVM subsystem scope. (Base 2.4, 3.1.4.1, Figure 36, page 55, PDF page 81)
};

/**
 * The values of Controller Ready Independent of Media Enable of CC - Controller Configuration
 */
enum xnvme_spec_base_registers_cc_crime {
	XNVME_SPEC_BASE_REGISTERS_CC_CRIME_CONTROLLER_READY_WITH_MEDIA_MODE = 0x0, ///< Controller Ready With Media Mode (Base 2.4, 3.1.4.5, Figure 41, page 60, PDF page 86)
	XNVME_SPEC_BASE_REGISTERS_CC_CRIME_CONTROLLER_READY_INDEPENDENT_OF_MEDIA_MODE = 0x1, ///< Controller Ready Independent Of Media Mode (Base 2.4, 3.1.4.5, Figure 41, page 60, PDF page 86)
};

/**
 * The values of Arbitration Mechanism Selected of CC - Controller Configuration
 */
enum xnvme_spec_base_registers_cc_ams {
	XNVME_SPEC_BASE_REGISTERS_CC_AMS_ROUND_ROBIN = 0x0, ///< Round Robin (Base 2.4, 3.1.4.5, Figure 41, page 60, PDF page 86)
	XNVME_SPEC_BASE_REGISTERS_CC_AMS_VENDOR_SPECIFIC = 0x7, ///< Vendor Specific (Base 2.4, 3.1.4.5, Figure 41, page 60, PDF page 86)
};

/**
 * The values of Shutdown Status of CSTS - Controller Status
 */
enum xnvme_spec_base_registers_csts_shst {
	XNVME_SPEC_BASE_REGISTERS_CSTS_SHST_NORMAL_OPERATION = 0x0, ///< Normal operation (Base 2.4, 3.1.4.6, Figure 42, page 63, PDF page 89)
};

/**
 * The values of Size Units of CMBSZ - Controller Memory Buffer Size
 */
enum xnvme_spec_base_registers_cmbsz_szu {
	XNVME_SPEC_BASE_REGISTERS_CMBSZ_SZU_4_KIB = 0x0, ///< 4 KiB (Base 2.4, 3.1.4.12, Figure 48, page 68, PDF page 94)
	XNVME_SPEC_BASE_REGISTERS_CMBSZ_SZU_64_KIB = 0x1, ///< 64 KiB (Base 2.4, 3.1.4.12, Figure 48, page 68, PDF page 94)
	XNVME_SPEC_BASE_REGISTERS_CMBSZ_SZU_1_MIB = 0x2, ///< 1 MiB (Base 2.4, 3.1.4.12, Figure 48, page 68, PDF page 94)
	XNVME_SPEC_BASE_REGISTERS_CMBSZ_SZU_16_MIB = 0x3, ///< 16 MiB (Base 2.4, 3.1.4.12, Figure 48, page 68, PDF page 94)
	XNVME_SPEC_BASE_REGISTERS_CMBSZ_SZU_256_MIB = 0x4, ///< 256 MiB (Base 2.4, 3.1.4.12, Figure 48, page 68, PDF page 94)
	XNVME_SPEC_BASE_REGISTERS_CMBSZ_SZU_4_GIB = 0x5, ///< 4 GiB (Base 2.4, 3.1.4.12, Figure 48, page 68, PDF page 94)
	XNVME_SPEC_BASE_REGISTERS_CMBSZ_SZU_64_GIB = 0x6, ///< 64 GiB (Base 2.4, 3.1.4.12, Figure 48, page 68, PDF page 94)
};

/**
 * The values of CMB Elasticity Buffer Size Units of CMBEBS - Controller Memory Buffer Elasticity Buffer Size
 */
enum xnvme_spec_base_registers_cmbebs_cmbszu {
	XNVME_SPEC_BASE_REGISTERS_CMBEBS_CMBSZU_BYTES = 0x0, ///< Bytes (Base 2.4, 3.1.4.18, Figure 54, page 71, PDF page 97)
	XNVME_SPEC_BASE_REGISTERS_CMBEBS_CMBSZU_1_KIB = 0x1, ///< 1 KiB (Base 2.4, 3.1.4.18, Figure 54, page 71, PDF page 97)
	XNVME_SPEC_BASE_REGISTERS_CMBEBS_CMBSZU_1_MIB = 0x2, ///< 1 MiB (Base 2.4, 3.1.4.18, Figure 54, page 71, PDF page 97)
	XNVME_SPEC_BASE_REGISTERS_CMBEBS_CMBSZU_1_GIB = 0x3, ///< 1 GiB (Base 2.4, 3.1.4.18, Figure 54, page 71, PDF page 97)
};

/**
 * The values of CMB Sustained Write Throughput Units of CMBSWTP - Controller Memory Buffer Sustained Write Throughput
 */
enum xnvme_spec_base_registers_cmbswtp_cmbswtu {
	XNVME_SPEC_BASE_REGISTERS_CMBSWTP_CMBSWTU_BYTES_SECOND = 0x0, ///< Bytes/second (Base 2.4, 3.1.4.19, Figure 55, page 72, PDF page 98)
	XNVME_SPEC_BASE_REGISTERS_CMBSWTP_CMBSWTU_1_KIB_SECOND = 0x1, ///< 1 KiB/second (Base 2.4, 3.1.4.19, Figure 55, page 72, PDF page 98)
	XNVME_SPEC_BASE_REGISTERS_CMBSWTP_CMBSWTU_1_MIB_SECOND = 0x2, ///< 1 MiB/second (Base 2.4, 3.1.4.19, Figure 55, page 72, PDF page 98)
	XNVME_SPEC_BASE_REGISTERS_CMBSWTP_CMBSWTU_1_GIB_SECOND = 0x3, ///< 1 GiB/second (Base 2.4, 3.1.4.19, Figure 55, page 72, PDF page 98)
};

/**
 * The values of Persistent Memory Region Time Units of PMRCAP - Persistent Memory Region Capabilities
 */
enum xnvme_spec_base_registers_pmrcap_pmrtu {
	XNVME_SPEC_BASE_REGISTERS_PMRCAP_PMRTU_500_MILLISECONDS = 0x0, ///< 500 milliseconds (Base 2.4, 3.1.4.22, Figure 58, page 73, PDF page 99)
	XNVME_SPEC_BASE_REGISTERS_PMRCAP_PMRTU_MINUTES = 0x1, ///< minutes (Base 2.4, 3.1.4.22, Figure 58, page 73, PDF page 99)
};

/**
 * The values of Health Status of PMRSTS - Persistent Memory Region Status
 */
enum xnvme_spec_base_registers_pmrsts_hsts {
	XNVME_SPEC_BASE_REGISTERS_PMRSTS_HSTS_NORMAL_OPERATION = 0x0, ///< Normal Operation (Base 2.4, 3.1.4.24, Figure 60, page 75, PDF page 101)
	XNVME_SPEC_BASE_REGISTERS_PMRSTS_HSTS_RESTORE_ERROR = 0x1, ///< Restore Error (Base 2.4, 3.1.4.24, Figure 60, page 75, PDF page 101)
	XNVME_SPEC_BASE_REGISTERS_PMRSTS_HSTS_READ_ONLY = 0x2, ///< Read Only (Base 2.4, 3.1.4.24, Figure 60, page 75, PDF page 101)
	XNVME_SPEC_BASE_REGISTERS_PMRSTS_HSTS_UNRELIABLE = 0x3, ///< Unreliable (Base 2.4, 3.1.4.24, Figure 60, page 75, PDF page 101)
};

/**
 * The values of PMR Elasticity Buffer Size Units of PMREBS - Persistent Memory Region Elasticity Buffer Size
 */
enum xnvme_spec_base_registers_pmrebs_pmrszu {
	XNVME_SPEC_BASE_REGISTERS_PMREBS_PMRSZU_BYTES = 0x0, ///< Bytes (Base 2.4, 3.1.4.25, Figure 61, page 76, PDF page 102)
	XNVME_SPEC_BASE_REGISTERS_PMREBS_PMRSZU_1_KIB = 0x1, ///< 1 KiB (Base 2.4, 3.1.4.25, Figure 61, page 76, PDF page 102)
	XNVME_SPEC_BASE_REGISTERS_PMREBS_PMRSZU_1_MIB = 0x2, ///< 1 MiB (Base 2.4, 3.1.4.25, Figure 61, page 76, PDF page 102)
	XNVME_SPEC_BASE_REGISTERS_PMREBS_PMRSZU_1_GIB = 0x3, ///< 1 GiB (Base 2.4, 3.1.4.25, Figure 61, page 76, PDF page 102)
};

/*
 * The fields of Delete I/O Submission Queue, by the dwords of the command
 */
/** Queue Identifier (Base 2.4, 5.3.4, Figure 582, page 528, PDF page 558); bits 15:0 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_DELETE_IO_SUBMISSION_QUEUE_CDW10_QID 0xffff

/*
 * The fields of Create I/O Submission Queue, by the dwords of the command
 */
/** Queue Size (Base 2.4, 5.3.2, Figure 576, page 526, PDF page 555); bits 31:16 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_CREATE_IO_SUBMISSION_QUEUE_CDW10_QSIZE 0xffff0000
/** Queue Identifier (Base 2.4, 5.3.2, Figure 576, page 526, PDF page 555); bits 15:0 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_CREATE_IO_SUBMISSION_QUEUE_CDW10_QID 0xffff
/** Completion Queue Identifier (Base 2.4, 5.3.2, Figure 577, page 526, PDF page 555); bits 31:16 of Command Dword 11 */
#define XNVME_SPEC_ADMIN_CREATE_IO_SUBMISSION_QUEUE_CDW11_CQID 0xffff0000
/** Queue Priority (Base 2.4, 5.3.2, Figure 577, page 526, PDF page 555); bits 2:1 of Command Dword 11 */
#define XNVME_SPEC_ADMIN_CREATE_IO_SUBMISSION_QUEUE_CDW11_QPRIO 0x6
/** Physically Contiguous (Base 2.4, 5.3.2, Figure 577, page 526, PDF page 555); bit 0 of Command Dword 11 */
#define XNVME_SPEC_ADMIN_CREATE_IO_SUBMISSION_QUEUE_CDW11_PC 0x1
/** NVM Set Identifier (Base 2.4, 5.3.2, Figure 578, page 527, PDF page 556); bits 15:0 of Command Dword 12 */
#define XNVME_SPEC_ADMIN_CREATE_IO_SUBMISSION_QUEUE_CDW12_NVMSETID 0xffff

/**
 * The values of Queue Priority (Base 2.4, 5.3.2, Figure 577, page 526, PDF page 555)
 */
enum xnvme_spec_admin_create_io_submission_queue_cdw11_qprio {
	XNVME_SPEC_ADMIN_CREATE_IO_SUBMISSION_QUEUE_CDW11_QPRIO_URGENT = 0x0, ///< Urgent (Base 2.4, 5.3.2, Figure 577, page 526, PDF page 555)
	XNVME_SPEC_ADMIN_CREATE_IO_SUBMISSION_QUEUE_CDW11_QPRIO_HIGH = 0x1, ///< High (Base 2.4, 5.3.2, Figure 577, page 526, PDF page 555)
	XNVME_SPEC_ADMIN_CREATE_IO_SUBMISSION_QUEUE_CDW11_QPRIO_MEDIUM = 0x2, ///< Medium (Base 2.4, 5.3.2, Figure 577, page 526, PDF page 555)
	XNVME_SPEC_ADMIN_CREATE_IO_SUBMISSION_QUEUE_CDW11_QPRIO_LOW = 0x3, ///< Low (Base 2.4, 5.3.2, Figure 577, page 526, PDF page 555)
};

/*
 * The fields of Get Log Page, by the dwords of the command
 */
/** Number of Dwords Lower (Base 2.4, 5.2.13, Figure 204, page 213, PDF page 239); bits 31:16 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_GET_LOG_PAGE_CDW10_NUMDL 0xffff0000
/** Retain Asynchronous Event (Base 2.4, 5.2.13, Figure 204, page 213, PDF page 239); bit 15 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_GET_LOG_PAGE_CDW10_RAE 0x8000
/** Log Specific Parameter (Base 2.4, 5.2.13, Figure 204, page 213, PDF page 239); bits 14:8 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_GET_LOG_PAGE_CDW10_LSP 0x7f00
/** Log Page Identifier (Base 2.4, 5.2.13, Figure 204, page 213, PDF page 239); bits 7:0 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_GET_LOG_PAGE_CDW10_LID 0xff
/** Log Specific Identifier (Base 2.4, 5.2.13, Figure 205, page 213, PDF page 240); bits 31:16 of Command Dword 11 */
#define XNVME_SPEC_ADMIN_GET_LOG_PAGE_CDW11_LSI 0xffff0000
/** Number of Dwords Upper (Base 2.4, 5.2.13, Figure 205, page 213, PDF page 240); bits 15:0 of Command Dword 11 */
#define XNVME_SPEC_ADMIN_GET_LOG_PAGE_CDW11_NUMDU 0xffff
/** Log Page Offset Lower (Base 2.4, 5.2.13, Figure 206, page 214, PDF page 240); bits 31:0 of Command Dword 12 */
#define XNVME_SPEC_ADMIN_GET_LOG_PAGE_CDW12_LPOL 0xffffffff
/** Log Page Offset Upper (Base 2.4, 5.2.13, Figure 207, page 214, PDF page 240); bits 31:0 of Command Dword 13 */
#define XNVME_SPEC_ADMIN_GET_LOG_PAGE_CDW13_LPOU 0xffffffff
/** Command Set Identifier (Base 2.4, 5.2.13, Figure 208, page 214, PDF page 240); bits 31:24 of Command Dword 14 */
#define XNVME_SPEC_ADMIN_GET_LOG_PAGE_CDW14_CSI 0xff000000
/** Offset Type (Base 2.4, 5.2.13, Figure 208, page 214, PDF page 240); bit 23 of Command Dword 14 */
#define XNVME_SPEC_ADMIN_GET_LOG_PAGE_CDW14_OT 0x800000
/** UUID Index (Base 2.4, 5.2.13, Figure 208, page 214, PDF page 240); bits 6:0 of Command Dword 14 */
#define XNVME_SPEC_ADMIN_GET_LOG_PAGE_CDW14_UIDX 0x7f

/**
 * The values of Log Page Identifier (Base 2.4, 5.2.13, Figure 209, page 215, PDF page 241)
 */
enum xnvme_spec_admin_get_log_page_cdw10_lid {
	XNVME_SPEC_ADMIN_GET_LOG_PAGE_CDW10_LID_SUPPORTED_LOG_PAGES = 0x0, ///< Supported Log Pages (Base 2.4, 5.2.13, Figure 209, page 215, PDF page 241)
	XNVME_SPEC_ADMIN_GET_LOG_PAGE_CDW10_LID_ERROR_INFORMATION = 0x1, ///< Error Information (Base 2.4, 5.2.13, Figure 209, page 215, PDF page 241)
	XNVME_SPEC_ADMIN_GET_LOG_PAGE_CDW10_LID_SMART_HEALTH_INFORMATION = 0x2, ///< SMART / Health Information (Base 2.4, 5.2.13, Figure 209, page 215, PDF page 241)
	XNVME_SPEC_ADMIN_GET_LOG_PAGE_CDW10_LID_FIRMWARE_SLOT_INFORMATION = 0x3, ///< Firmware Slot Information (Base 2.4, 5.2.13, Figure 209, page 215, PDF page 241)
	XNVME_SPEC_ADMIN_GET_LOG_PAGE_CDW10_LID_CHANGED_ATTACHED_NAMESPACE_LIST = 0x4, ///< Changed Attached Namespace List (Base 2.4, 5.2.13, Figure 209, page 215, PDF page 241)
	XNVME_SPEC_ADMIN_GET_LOG_PAGE_CDW10_LID_COMMANDS_SUPPORTED_AND_EFFECTS = 0x5, ///< Commands Supported and Effects (Base 2.4, 5.2.13, Figure 209, page 215, PDF page 241)
	XNVME_SPEC_ADMIN_GET_LOG_PAGE_CDW10_LID_DEVICE_SELF_TEST_5 = 0x6, ///< Device Self-test 5 (Base 2.4, 5.2.13, Figure 209, page 215, PDF page 241)
	XNVME_SPEC_ADMIN_GET_LOG_PAGE_CDW10_LID_TELEMETRY_HOST_INITIATED_5 = 0x7, ///< Telemetry Host-Initiated 5 (Base 2.4, 5.2.13, Figure 209, page 215, PDF page 241)
	XNVME_SPEC_ADMIN_GET_LOG_PAGE_CDW10_LID_TELEMETRY_CONTROLLER_INITIATED_5 = 0x8, ///< Telemetry Controller-Initiated 5 (Base 2.4, 5.2.13, Figure 209, page 215, PDF page 241)
	XNVME_SPEC_ADMIN_GET_LOG_PAGE_CDW10_LID_ENDURANCE_GROUP_INFORMATION = 0x9, ///< Endurance Group Information (Base 2.4, 5.2.13, Figure 209, page 215, PDF page 241)
	XNVME_SPEC_ADMIN_GET_LOG_PAGE_CDW10_LID_PREDICTABLE_LATENCY_PER_NVM_SET = 0xa, ///< Predictable Latency Per NVM Set (Base 2.4, 5.2.13, Figure 209, page 215, PDF page 241)
	XNVME_SPEC_ADMIN_GET_LOG_PAGE_CDW10_LID_PREDICTABLE_LATENCY_EVENT_AGGREGATE = 0xb, ///< Predictable Latency Event Aggregate (Base 2.4, 5.2.13, Figure 209, page 215, PDF page 241)
	XNVME_SPEC_ADMIN_GET_LOG_PAGE_CDW10_LID_ASYMMETRIC_NAMESPACE_ACCESS = 0xc, ///< Asymmetric Namespace Access (Base 2.4, 5.2.13, Figure 209, page 215, PDF page 241)
	XNVME_SPEC_ADMIN_GET_LOG_PAGE_CDW10_LID_PERSISTENT_EVENT_LOG_5 = 0xd, ///< Persistent Event Log 5 (Base 2.4, 5.2.13, Figure 209, page 215, PDF page 241)
	XNVME_SPEC_ADMIN_GET_LOG_PAGE_CDW10_LID_ENDURANCE_GROUP_EVENT_AGGREGATE = 0xf, ///< Endurance Group Event Aggregate (Base 2.4, 5.2.13, Figure 209, page 215, PDF page 241)
	XNVME_SPEC_ADMIN_GET_LOG_PAGE_CDW10_LID_MEDIA_UNIT_STATUS = 0x10, ///< Media Unit Status (Base 2.4, 5.2.13, Figure 209, page 215, PDF page 241)
	XNVME_SPEC_ADMIN_GET_LOG_PAGE_CDW10_LID_SUPPORTED_CAPACITY_CONFIGURATION_LIST = 0x11, ///< Supported Capacity Configuration List (Base 2.4, 5.2.13, Figure 209, page 215, PDF page 241)
	XNVME_SPEC_ADMIN_GET_LOG_PAGE_CDW10_LID_FEATURE_IDENTIFIERS_SUPPORTED_AND_EFFECTS = 0x12, ///< Feature Identifiers Supported and Effects (Base 2.4, 5.2.13, Figure 209, page 215, PDF page 241)
	XNVME_SPEC_ADMIN_GET_LOG_PAGE_CDW10_LID_NVME_MI_COMMANDS_SUPPORTED_AND_EFFECTS = 0x13, ///< NVMe-MI Commands Supported and Effects (Base 2.4, 5.2.13, Figure 209, page 215, PDF page 241)
	XNVME_SPEC_ADMIN_GET_LOG_PAGE_CDW10_LID_COMMAND_AND_FEATURE_LOCKDOWN_5 = 0x14, ///< Command and Feature Lockdown 5 (Base 2.4, 5.2.13, Figure 209, page 215, PDF page 241)
	XNVME_SPEC_ADMIN_GET_LOG_PAGE_CDW10_LID_BOOT_PARTITION = 0x15, ///< Boot Partition (Base 2.4, 5.2.13, Figure 209, page 215, PDF page 241)
	XNVME_SPEC_ADMIN_GET_LOG_PAGE_CDW10_LID_ROTATIONAL_MEDIA_INFORMATION = 0x16, ///< Rotational Media Information (Base 2.4, 5.2.13, Figure 209, page 215, PDF page 241)
	XNVME_SPEC_ADMIN_GET_LOG_PAGE_CDW10_LID_DISPERSED_NAMESPACE_PARTICIPATING_NVM_SUBSYSTEMS = 0x17, ///< Dispersed Namespace Participating NVM Subsystems (Base 2.4, 5.2.13, Figure 209, page 215, PDF page 241)
	XNVME_SPEC_ADMIN_GET_LOG_PAGE_CDW10_LID_MANAGEMENT_ADDRESS_LIST = 0x18, ///< Management Address List (Base 2.4, 5.2.13, Figure 209, page 215, PDF page 241)
	XNVME_SPEC_ADMIN_GET_LOG_PAGE_CDW10_LID_REACHABILITY_GROUPS = 0x1a, ///< Reachability Groups (Base 2.4, 5.2.13, Figure 209, page 215, PDF page 241)
	XNVME_SPEC_ADMIN_GET_LOG_PAGE_CDW10_LID_REACHABILITY_ASSOCIATIONS = 0x1b, ///< Reachability Associations (Base 2.4, 5.2.13, Figure 209, page 215, PDF page 241)
	XNVME_SPEC_ADMIN_GET_LOG_PAGE_CDW10_LID_CHANGED_ALLOCATED_NAMESPACE_LIST = 0x1c, ///< Changed Allocated Namespace List (Base 2.4, 5.2.13, Figure 209, page 215, PDF page 241)
	XNVME_SPEC_ADMIN_GET_LOG_PAGE_CDW10_LID_DEVICE_PERSONALITIES = 0x1d, ///< Device Personalities (Base 2.4, 5.2.13, Figure 209, page 215, PDF page 241)
	XNVME_SPEC_ADMIN_GET_LOG_PAGE_CDW10_LID_CROSS_CONTROLLER_RESET = 0x1e, ///< Cross-Controller Reset (Base 2.4, 5.2.13, Figure 209, page 215, PDF page 241)
	XNVME_SPEC_ADMIN_GET_LOG_PAGE_CDW10_LID_LOST_HOST_COMMUNICATION = 0x1f, ///< Lost Host Communication (Base 2.4, 5.2.13, Figure 209, page 215, PDF page 241)
	XNVME_SPEC_ADMIN_GET_LOG_PAGE_CDW10_LID_FDP_CONFIGURATIONS = 0x20, ///< FDP Configurations (Base 2.4, 5.2.13, Figure 209, page 215, PDF page 241)
	XNVME_SPEC_ADMIN_GET_LOG_PAGE_CDW10_LID_RECLAIM_UNIT_HANDLE_USAGE = 0x21, ///< Reclaim Unit Handle Usage (Base 2.4, 5.2.13, Figure 209, page 215, PDF page 241)
	XNVME_SPEC_ADMIN_GET_LOG_PAGE_CDW10_LID_FDP_STATISTICS = 0x22, ///< FDP Statistics (Base 2.4, 5.2.13, Figure 209, page 215, PDF page 241)
	XNVME_SPEC_ADMIN_GET_LOG_PAGE_CDW10_LID_FDP_EVENTS = 0x23, ///< FDP Events (Base 2.4, 5.2.13, Figure 209, page 215, PDF page 241)
	XNVME_SPEC_ADMIN_GET_LOG_PAGE_CDW10_LID_MANUFACTURER_DEFAULT_CONFIGURATION = 0x24, ///< Manufacturer Default Configuration (Base 2.4, 5.2.13, Figure 209, page 215, PDF page 241)
	XNVME_SPEC_ADMIN_GET_LOG_PAGE_CDW10_LID_POWER_MEASUREMENT = 0x25, ///< Power Measurement (Base 2.4, 5.2.13, Figure 209, page 215, PDF page 241)
	XNVME_SPEC_ADMIN_GET_LOG_PAGE_CDW10_LID_VOLTAGE_MEASUREMENT = 0x27, ///< Voltage Measurement (Base 2.4, 5.2.13, Figure 209, page 215, PDF page 241)
	XNVME_SPEC_ADMIN_GET_LOG_PAGE_CDW10_LID_DISCOVERY = 0x70, ///< Discovery (Base 2.4, 5.2.13, Figure 209, page 215, PDF page 241)
	XNVME_SPEC_ADMIN_GET_LOG_PAGE_CDW10_LID_HOST_DISCOVERY = 0x71, ///< Host Discovery (Base 2.4, 5.2.13, Figure 209, page 215, PDF page 241)
	XNVME_SPEC_ADMIN_GET_LOG_PAGE_CDW10_LID_AVE_DISCOVERY = 0x72, ///< AVE Discovery (Base 2.4, 5.2.13, Figure 209, page 215, PDF page 241)
	XNVME_SPEC_ADMIN_GET_LOG_PAGE_CDW10_LID_PULL_MODEL_DDC_REQUEST = 0x73, ///< Pull Model DDC Request (Base 2.4, 5.2.13, Figure 209, page 215, PDF page 241)
	XNVME_SPEC_ADMIN_GET_LOG_PAGE_CDW10_LID_SANITIZE_NAMESPACE_STATUS_LIST = 0x7f, ///< Sanitize Namespace Status List (Base 2.4, 5.2.13, Figure 209, page 215, PDF page 241)
	XNVME_SPEC_ADMIN_GET_LOG_PAGE_CDW10_LID_RESERVATION_NOTIFICATION = 0x80, ///< Reservation Notification (Base 2.4, 5.2.13, Figure 209, page 215, PDF page 241)
	XNVME_SPEC_ADMIN_GET_LOG_PAGE_CDW10_LID_SANITIZE_STATUS = 0x81, ///< Sanitize Status (Base 2.4, 5.2.13, Figure 209, page 215, PDF page 241)
};

/**
 * The values of Command Set Identifier (Base 2.4, 5.2.14.1, Figure 337, page 338, PDF page 366)
 */
enum xnvme_spec_admin_get_log_page_cdw14_csi {
	XNVME_SPEC_ADMIN_GET_LOG_PAGE_CDW14_CSI_NVM_COMMAND_SET = 0x0, ///< NVM Command Set (Base 2.4, 5.2.14.1, Figure 337, page 338, PDF page 366)
	XNVME_SPEC_ADMIN_GET_LOG_PAGE_CDW14_CSI_KEY_VALUE_COMMAND_SET = 0x1, ///< Key Value Command Set (Base 2.4, 5.2.14.1, Figure 337, page 338, PDF page 366)
	XNVME_SPEC_ADMIN_GET_LOG_PAGE_CDW14_CSI_ZONED_NAMESPACE_COMMAND_SET = 0x2, ///< Zoned Namespace Command Set (Base 2.4, 5.2.14.1, Figure 337, page 338, PDF page 366)
	XNVME_SPEC_ADMIN_GET_LOG_PAGE_CDW14_CSI_SUBSYSTEM_LOCAL_MEMORY = 0x3, ///< Subsystem Local Memory (Base 2.4, 5.2.14.1, Figure 337, page 338, PDF page 366)
	XNVME_SPEC_ADMIN_GET_LOG_PAGE_CDW14_CSI_COMPUTATIONAL_PROGRAMS_COMMAND = 0x4, ///< Computational Programs Command (Base 2.4, 5.2.14.1, Figure 337, page 338, PDF page 366)
};

/*
 * The fields of Delete I/O Completion Queue, by the dwords of the command
 */
/** Queue Identifier (Base 2.4, 5.3.3, Figure 580, page 528, PDF page 557); bits 15:0 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_DELETE_IO_COMPLETION_QUEUE_CDW10_QID 0xffff

/*
 * The fields of Create I/O Completion Queue, by the dwords of the command
 */
/** Queue Size (Base 2.4, 5.3.1, Figure 572, page 524, PDF page 554); bits 31:16 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_CREATE_IO_COMPLETION_QUEUE_CDW10_QSIZE 0xffff0000
/** Queue Identifier (Base 2.4, 5.3.1, Figure 572, page 524, PDF page 554); bits 15:0 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_CREATE_IO_COMPLETION_QUEUE_CDW10_QID 0xffff
/** Interrupt Vector (Base 2.4, 5.3.1, Figure 573, page 524, PDF page 554); bits 31:16 of Command Dword 11 */
#define XNVME_SPEC_ADMIN_CREATE_IO_COMPLETION_QUEUE_CDW11_IV 0xffff0000
/** Interrupts Enabled (Base 2.4, 5.3.1, Figure 573, page 524, PDF page 554); bit 1 of Command Dword 11 */
#define XNVME_SPEC_ADMIN_CREATE_IO_COMPLETION_QUEUE_CDW11_IEN 0x2
/** Physically Contiguous (Base 2.4, 5.3.1, Figure 573, page 524, PDF page 554); bit 0 of Command Dword 11 */
#define XNVME_SPEC_ADMIN_CREATE_IO_COMPLETION_QUEUE_CDW11_PC 0x1

/*
 * The fields of Identify, by the dwords of the command
 */
/** Controller Identifier (Base 2.4, 5.2.14.1, Figure 333, page 335, PDF page 363); bits 31:16 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_IDENTIFY_CDW10_CNTID 0xffff0000
/** Controller or Namespace Structure (Base 2.4, 5.2.14.1, Figure 333, page 335, PDF page 363); bits 7:0 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_IDENTIFY_CDW10_CNS 0xff
/** Command Set Identifier (Base 2.4, 5.2.14.1, Figure 334, page 335, PDF page 363); bits 31:24 of Command Dword 11 */
#define XNVME_SPEC_ADMIN_IDENTIFY_CDW11_CSI 0xff000000
/** CNS Specific Identifier (Base 2.4, 5.2.14.1, Figure 334, page 335, PDF page 363); bits 15:0 of Command Dword 11 */
#define XNVME_SPEC_ADMIN_IDENTIFY_CDW11_CNSSID 0xffff
/** UUID Index (Base 2.4, 5.2.14.1, Figure 335, page 335, PDF page 363); bits 6:0 of Command Dword 14 */
#define XNVME_SPEC_ADMIN_IDENTIFY_CDW14_UIDX 0x7f

/**
 * The values of Controller or Namespace Structure (Base 2.4, 5.2.14.1, Figure 336, page 336, PDF page 364)
 */
enum xnvme_spec_admin_identify_cdw10_cns {
	XNVME_SPEC_ADMIN_IDENTIFY_CDW10_CNS_DOMAIN_LIST = 0x18, ///< Domain List (Base 2.4, 5.2.14.1, Figure 336, page 336, PDF page 364)
	XNVME_SPEC_ADMIN_IDENTIFY_CDW10_CNS_ENDURANCE_GROUP_LIST = 0x19, ///< Endurance Group List (Base 2.4, 5.2.14.1, Figure 336, page 336, PDF page 364)
	XNVME_SPEC_ADMIN_IDENTIFY_CDW10_CNS_GET_UNDERLYING_NAMESPACE_LIST = 0x1d, ///< Get Underlying Namespace List (Base 2.4, 5.2.14.1, Figure 336, page 336, PDF page 364)
	XNVME_SPEC_ADMIN_IDENTIFY_CDW10_CNS_GET_PORTS_LIST = 0x1e, ///< Get Ports List (Base 2.4, 5.2.14.1, Figure 336, page 336, PDF page 364)
	XNVME_SPEC_ADMIN_IDENTIFY_CDW10_CNS_SUPPORTED_CONTROLLER_STATE_FORMATS = 0x20, ///< Supported Controller State Formats (Base 2.4, 5.2.14.1, Figure 336, page 336, PDF page 364)
	XNVME_SPEC_ADMIN_IDENTIFY_CDW10_CNS_GET_UNDERLYING_CONTROLLER_LIST = 0x21, ///< Get Underlying Controller List (Base 2.4, 5.2.14.1, Figure 336, page 336, PDF page 364)
};

/**
 * The values of Command Set Identifier (Base 2.4, 5.2.14.1, Figure 337, page 338, PDF page 366)
 */
enum xnvme_spec_admin_identify_cdw11_csi {
	XNVME_SPEC_ADMIN_IDENTIFY_CDW11_CSI_NVM_COMMAND_SET = 0x0, ///< NVM Command Set (Base 2.4, 5.2.14.1, Figure 337, page 338, PDF page 366)
	XNVME_SPEC_ADMIN_IDENTIFY_CDW11_CSI_KEY_VALUE_COMMAND_SET = 0x1, ///< Key Value Command Set (Base 2.4, 5.2.14.1, Figure 337, page 338, PDF page 366)
	XNVME_SPEC_ADMIN_IDENTIFY_CDW11_CSI_ZONED_NAMESPACE_COMMAND_SET = 0x2, ///< Zoned Namespace Command Set (Base 2.4, 5.2.14.1, Figure 337, page 338, PDF page 366)
	XNVME_SPEC_ADMIN_IDENTIFY_CDW11_CSI_SUBSYSTEM_LOCAL_MEMORY = 0x3, ///< Subsystem Local Memory (Base 2.4, 5.2.14.1, Figure 337, page 338, PDF page 366)
	XNVME_SPEC_ADMIN_IDENTIFY_CDW11_CSI_COMPUTATIONAL_PROGRAMS_COMMAND = 0x4, ///< Computational Programs Command (Base 2.4, 5.2.14.1, Figure 337, page 338, PDF page 366)
};

/*
 * The fields of Abort, by the dwords of the command
 */
/** Command Identifier (Base 2.4, 5.2.1, Figure 147, page 182, PDF page 208); bits 31:16 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_ABORT_CDW10_CID 0xffff0000
/** Submission Queue Identifier (Base 2.4, 5.2.1, Figure 147, page 182, PDF page 208); bits 15:0 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_ABORT_CDW10_SQID 0xffff

/*
 * The fields of Set Features, by the dwords of the command
 */
/** Save (Base 2.4, 5.2.30, Figure 464, page 454, PDF page 483); bit 31 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_SET_FEATURES_CDW10_SV 0x80000000
/** Feature Identifier (Base 2.4, 5.2.30, Figure 464, page 454, PDF page 483); bits 7:0 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_SET_FEATURES_CDW10_FID 0xff
/** Element Action (Base 2.4, 5.2.30.1.33, Figure 531, page 501, PDF page 530); bits 14:13 of Command Dword 11 */
#define XNVME_SPEC_ADMIN_SET_FEATURES_CDW11_EA 0x6000
/** UUID Index (Base 2.4, 5.2.30, Figure 465, page 455, PDF page 483); bits 6:0 of Command Dword 14 */
#define XNVME_SPEC_ADMIN_SET_FEATURES_CDW14_UIDX 0x7f

/**
 * The values of Element Action (Base 2.4, 5.2.30.1.33, Figure 531, page 501, PDF page 530)
 */
enum xnvme_spec_admin_set_features_cdw11_ea {
	XNVME_SPEC_ADMIN_SET_FEATURES_CDW11_EA_ADD_OR_REPLACE_ENTRY = 0x0, ///< Add or Replace Entry (Base 2.4, 5.2.30.1.33, Figure 531, page 501, PDF page 530)
	XNVME_SPEC_ADMIN_SET_FEATURES_CDW11_EA_DELETE_ENTRY_MULTIPLE = 0x1, ///< Delete Entry Multiple (Base 2.4, 5.2.30.1.33, Figure 531, page 501, PDF page 530)
	XNVME_SPEC_ADMIN_SET_FEATURES_CDW11_EA_ADD_ENTRY_MULTIPLE = 0x2, ///< Add Entry Multiple (Base 2.4, 5.2.30.1.33, Figure 531, page 501, PDF page 530)
};

/*
 * The fields of Get Features, by the dwords of the command
 */
/** Select (Base 2.4, 5.2.12, Figure 198, page 209, PDF page 235); bits 10:8 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_GET_FEATURES_CDW10_SEL 0x700
/** Feature Identifier (Base 2.4, 5.2.12, Figure 198, page 209, PDF page 235); bits 7:0 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_GET_FEATURES_CDW10_FID 0xff
/** Generate Default Host Metadata (Base 2.4, 5.2.30.1.33, Figure 530, page 500, PDF page 529); bit 0 of Command Dword 11 */
#define XNVME_SPEC_ADMIN_GET_FEATURES_CDW11_GDHM 0x1
/** UUID Index (Base 2.4, 5.2.12, Figure 199, page 210, PDF page 236); bits 6:0 of Command Dword 14 */
#define XNVME_SPEC_ADMIN_GET_FEATURES_CDW14_UIDX 0x7f

/**
 * The values of Select (Base 2.4, 5.2.12, Figure 198, page 209, PDF page 235)
 */
enum xnvme_spec_admin_get_features_cdw10_sel {
	XNVME_SPEC_ADMIN_GET_FEATURES_CDW10_SEL_CURRENT = 0x0, ///< Current (Base 2.4, 5.2.12, Figure 198, page 209, PDF page 235)
	XNVME_SPEC_ADMIN_GET_FEATURES_CDW10_SEL_DEFAULT = 0x1, ///< Default (Base 2.4, 5.2.12, Figure 198, page 209, PDF page 235)
	XNVME_SPEC_ADMIN_GET_FEATURES_CDW10_SEL_SAVED = 0x2, ///< Saved (Base 2.4, 5.2.12, Figure 198, page 209, PDF page 235)
	XNVME_SPEC_ADMIN_GET_FEATURES_CDW10_SEL_SUPPORTED_CAPABILITIES = 0x3, ///< Supported Capabilities (Base 2.4, 5.2.12, Figure 198, page 209, PDF page 235)
};

/*
 * The fields of Asynchronous Event Request, by the dwords of the command
 */

/*
 * The fields of Namespace Management, by the dwords of the command
 */
/** Select (Base 2.4, 5.2.25, Figure 446, page 444, PDF page 472); bits 3:0 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_NAMESPACE_MANAGEMENT_CDW10_SEL 0xf
/** Command Set Identifier (Base 2.4, 5.2.25, Figure 447, page 445, PDF page 473); bits 31:24 of Command Dword 11 */
#define XNVME_SPEC_ADMIN_NAMESPACE_MANAGEMENT_CDW11_CSI 0xff000000

/**
 * The values of Select (Base 2.4, 5.2.25, Figure 446, page 444, PDF page 472)
 */
enum xnvme_spec_admin_namespace_management_cdw10_sel {
	XNVME_SPEC_ADMIN_NAMESPACE_MANAGEMENT_CDW10_SEL_CREATE = 0x0, ///< Create (Base 2.4, 5.2.25, Figure 446, page 444, PDF page 472)
	XNVME_SPEC_ADMIN_NAMESPACE_MANAGEMENT_CDW10_SEL_DELETE = 0x1, ///< Delete (Base 2.4, 5.2.25, Figure 446, page 444, PDF page 472)
	XNVME_SPEC_ADMIN_NAMESPACE_MANAGEMENT_CDW10_SEL_RESTORE_DEFAULT_NAMESPACE_CONFIGURATION = 0x2, ///< Restore Default Namespace Configuration (Base 2.4, 5.2.25, Figure 446, page 444, PDF page 472)
};

/**
 * The values of Command Set Identifier (Base 2.4, 5.2.14.1, Figure 337, page 338, PDF page 366)
 */
enum xnvme_spec_admin_namespace_management_cdw11_csi {
	XNVME_SPEC_ADMIN_NAMESPACE_MANAGEMENT_CDW11_CSI_NVM_COMMAND_SET = 0x0, ///< NVM Command Set (Base 2.4, 5.2.14.1, Figure 337, page 338, PDF page 366)
	XNVME_SPEC_ADMIN_NAMESPACE_MANAGEMENT_CDW11_CSI_KEY_VALUE_COMMAND_SET = 0x1, ///< Key Value Command Set (Base 2.4, 5.2.14.1, Figure 337, page 338, PDF page 366)
	XNVME_SPEC_ADMIN_NAMESPACE_MANAGEMENT_CDW11_CSI_ZONED_NAMESPACE_COMMAND_SET = 0x2, ///< Zoned Namespace Command Set (Base 2.4, 5.2.14.1, Figure 337, page 338, PDF page 366)
	XNVME_SPEC_ADMIN_NAMESPACE_MANAGEMENT_CDW11_CSI_SUBSYSTEM_LOCAL_MEMORY = 0x3, ///< Subsystem Local Memory (Base 2.4, 5.2.14.1, Figure 337, page 338, PDF page 366)
	XNVME_SPEC_ADMIN_NAMESPACE_MANAGEMENT_CDW11_CSI_COMPUTATIONAL_PROGRAMS_COMMAND = 0x4, ///< Computational Programs Command (Base 2.4, 5.2.14.1, Figure 337, page 338, PDF page 366)
};

/*
 * The fields of Firmware Commit, by the dwords of the command
 */
/** Boot Partition ID (Base 2.4, 5.2.9, Figure 187, page 203, PDF page 229); bit 31 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_FIRMWARE_COMMIT_CDW10_BPID 0x80000000
/** Commit Action (Base 2.4, 5.2.9, Figure 187, page 203, PDF page 229); bits 5:3 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_FIRMWARE_COMMIT_CDW10_CA 0x38
/** Firmware Slot (Base 2.4, 5.2.9, Figure 187, page 203, PDF page 229); bits 2:0 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_FIRMWARE_COMMIT_CDW10_FS 0x7

/*
 * The fields of Firmware Image Download, by the dwords of the command
 */
/** Number of Dwords (Base 2.4, 5.2.10, Figure 191, page 205, PDF page 231); bits 31:0 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_FIRMWARE_IMAGE_DOWNLOAD_CDW10_NUMD 0xffffffff
/** Offset (Base 2.4, 5.2.10, Figure 192, page 206, PDF page 232); bits 31:0 of Command Dword 11 */
#define XNVME_SPEC_ADMIN_FIRMWARE_IMAGE_DOWNLOAD_CDW11_OFST 0xffffffff

/*
 * The fields of Device Self-test, by the dwords of the command
 */
/** Self-test Code (Base 2.4, 5.2.6, Figure 177, page 199, PDF page 225); bits 3:0 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_DEVICE_SELF_TEST_CDW10_STC 0xf
/** Device Self-test Parameter (Base 2.4, 5.2.6, Figure 178, page 200, PDF page 226); bits 31:0 of Command Dword 15 */
#define XNVME_SPEC_ADMIN_DEVICE_SELF_TEST_CDW15_DSTP 0xffffffff

/*
 * The fields of Namespace Attachment, by the dwords of the command
 */
/** Select (Base 2.4, 5.2.24, Figure 443, page 443, PDF page 471); bits 3:0 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_NAMESPACE_ATTACHMENT_CDW10_SEL 0xf

/**
 * The values of Select (Base 2.4, 5.2.24, Figure 443, page 443, PDF page 471)
 */
enum xnvme_spec_admin_namespace_attachment_cdw10_sel {
	XNVME_SPEC_ADMIN_NAMESPACE_ATTACHMENT_CDW10_SEL_CONTROLLER_ATTACH = 0x0, ///< Controller Attach (Base 2.4, 5.2.24, Figure 443, page 443, PDF page 471)
	XNVME_SPEC_ADMIN_NAMESPACE_ATTACHMENT_CDW10_SEL_CONTROLLER_DETACH = 0x1, ///< Controller Detach (Base 2.4, 5.2.24, Figure 443, page 443, PDF page 471)
};

/*
 * The fields of Keep Alive, by the dwords of the command
 */

/*
 * The fields of Directive Send, by the dwords of the command
 */
/** Number of Dwords (Base 2.4, 5.2.8, Figure 185, page 202, PDF page 228); bits 31:0 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_DIRECTIVE_SEND_CDW10_NUMD 0xffffffff
/** Directive Specific (Base 2.4, 5.2.8, Figure 186, page 202, PDF page 228); bits 31:16 of Command Dword 11 */
#define XNVME_SPEC_ADMIN_DIRECTIVE_SEND_CDW11_DSPEC 0xffff0000
/** Directive Type (Base 2.4, 5.2.8, Figure 186, page 202, PDF page 228); bits 15:8 of Command Dword 11 */
#define XNVME_SPEC_ADMIN_DIRECTIVE_SEND_CDW11_DTYPE 0xff00
/** Directive Operation (Base 2.4, 5.2.8, Figure 186, page 202, PDF page 228); bits 7:0 of Command Dword 11 */
#define XNVME_SPEC_ADMIN_DIRECTIVE_SEND_CDW11_DOPER 0xff

/**
 * The values of Directive Type (Base 2.4, 8.1.9, Figure 702, page 613, PDF page 643)
 */
enum xnvme_spec_admin_directive_send_cdw11_dtype {
	XNVME_SPEC_ADMIN_DIRECTIVE_SEND_CDW11_DTYPE_IDENTIFY = 0x0, ///< Identify (Base 2.4, 8.1.9, Figure 702, page 613, PDF page 643)
	XNVME_SPEC_ADMIN_DIRECTIVE_SEND_CDW11_DTYPE_STREAMS = 0x1, ///< Streams (Base 2.4, 8.1.9, Figure 702, page 613, PDF page 643)
	XNVME_SPEC_ADMIN_DIRECTIVE_SEND_CDW11_DTYPE_DATA_PLACEMENT = 0x2, ///< Data Placement (Base 2.4, 8.1.9, Figure 702, page 613, PDF page 643)
	XNVME_SPEC_ADMIN_DIRECTIVE_SEND_CDW11_DTYPE_VENDOR_SPECIFIC = 0xf, ///< Vendor Specific (Base 2.4, 8.1.9, Figure 702, page 613, PDF page 643)
};

/**
 * The values of DOPER of Directive Send when DTYPE is 0h
 */
enum xnvme_spec_admin_directive_send_cdw11_doper_dtype0h {
	XNVME_SPEC_ADMIN_DIRECTIVE_SEND_CDW11_DOPER_DTYPE0H_ENABLE_DIRECTIVE = 0x1, ///< Enable Directive (Base 2.4, 8.1.9.2, Figure 704, page 614, PDF page 644)
};

/**
 * The values of DOPER of Directive Send when DTYPE is 1h
 */
enum xnvme_spec_admin_directive_send_cdw11_doper_dtype1h {
	XNVME_SPEC_ADMIN_DIRECTIVE_SEND_CDW11_DOPER_DTYPE1H_RELEASE_IDENTIFIER = 0x1, ///< Release Identifier (Base 2.4, 8.1.9.3, Figure 709, page 617, PDF page 647)
	XNVME_SPEC_ADMIN_DIRECTIVE_SEND_CDW11_DOPER_DTYPE1H_RELEASE_RESOURCES = 0x2, ///< Release Resources (Base 2.4, 8.1.9.3, Figure 709, page 617, PDF page 647)
};

/*
 * The fields of Directive Receive, by the dwords of the command
 */
/** Number of Dwords (Base 2.4, 5.2.7, Figure 182, page 201, PDF page 227); bits 31:0 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_DIRECTIVE_RECEIVE_CDW10_NUMD 0xffffffff
/** Directive Specific (Base 2.4, 5.2.7, Figure 183, page 201, PDF page 227); bits 31:16 of Command Dword 11 */
#define XNVME_SPEC_ADMIN_DIRECTIVE_RECEIVE_CDW11_DSPEC 0xffff0000
/** Directive Type (Base 2.4, 5.2.7, Figure 183, page 201, PDF page 227); bits 15:8 of Command Dword 11 */
#define XNVME_SPEC_ADMIN_DIRECTIVE_RECEIVE_CDW11_DTYPE 0xff00
/** Directive Operation (Base 2.4, 5.2.7, Figure 183, page 201, PDF page 227); bits 7:0 of Command Dword 11 */
#define XNVME_SPEC_ADMIN_DIRECTIVE_RECEIVE_CDW11_DOPER 0xff

/**
 * The values of Directive Type (Base 2.4, 8.1.9, Figure 702, page 613, PDF page 643)
 */
enum xnvme_spec_admin_directive_receive_cdw11_dtype {
	XNVME_SPEC_ADMIN_DIRECTIVE_RECEIVE_CDW11_DTYPE_IDENTIFY = 0x0, ///< Identify (Base 2.4, 8.1.9, Figure 702, page 613, PDF page 643)
	XNVME_SPEC_ADMIN_DIRECTIVE_RECEIVE_CDW11_DTYPE_STREAMS = 0x1, ///< Streams (Base 2.4, 8.1.9, Figure 702, page 613, PDF page 643)
	XNVME_SPEC_ADMIN_DIRECTIVE_RECEIVE_CDW11_DTYPE_DATA_PLACEMENT = 0x2, ///< Data Placement (Base 2.4, 8.1.9, Figure 702, page 613, PDF page 643)
	XNVME_SPEC_ADMIN_DIRECTIVE_RECEIVE_CDW11_DTYPE_VENDOR_SPECIFIC = 0xf, ///< Vendor Specific (Base 2.4, 8.1.9, Figure 702, page 613, PDF page 643)
};

/**
 * The values of DOPER of Directive Receive when DTYPE is 0h
 */
enum xnvme_spec_admin_directive_receive_cdw11_doper_dtype0h {
	XNVME_SPEC_ADMIN_DIRECTIVE_RECEIVE_CDW11_DOPER_DTYPE0H_RETURN_PARAMETERS = 0x1, ///< Return Parameters (Base 2.4, 8.1.9.2, Figure 704, page 614, PDF page 644)
};

/**
 * The values of DOPER of Directive Receive when DTYPE is 1h
 */
enum xnvme_spec_admin_directive_receive_cdw11_doper_dtype1h {
	XNVME_SPEC_ADMIN_DIRECTIVE_RECEIVE_CDW11_DOPER_DTYPE1H_RETURN_PARAMETERS = 0x1, ///< Return Parameters (Base 2.4, 8.1.9.3, Figure 709, page 617, PDF page 647)
	XNVME_SPEC_ADMIN_DIRECTIVE_RECEIVE_CDW11_DOPER_DTYPE1H_GET_STATUS = 0x2, ///< Get Status (Base 2.4, 8.1.9.3, Figure 709, page 617, PDF page 647)
	XNVME_SPEC_ADMIN_DIRECTIVE_RECEIVE_CDW11_DOPER_DTYPE1H_ALLOCATE_RESOURCES = 0x3, ///< Allocate Resources (Base 2.4, 8.1.9.3, Figure 709, page 617, PDF page 647)
};

/*
 * The fields of Virtualization Management, by the dwords of the command
 */
/** Controller Identifier (Base 2.4, 5.3.6, Figure 587, page 530, PDF page 559); bits 31:16 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_VIRTUALIZATION_MANAGEMENT_CDW10_CNTLID 0xffff0000
/** Resource Type (Base 2.4, 5.3.6, Figure 587, page 530, PDF page 559); bits 10:8 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_VIRTUALIZATION_MANAGEMENT_CDW10_RT 0x700
/** Action (Base 2.4, 5.3.6, Figure 587, page 530, PDF page 559); bits 3:0 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_VIRTUALIZATION_MANAGEMENT_CDW10_ACT 0xf
/** Number of Controller Resources (Base 2.4, 5.3.6, Figure 588, page 531, PDF page 560); bits 15:0 of Command Dword 11 */
#define XNVME_SPEC_ADMIN_VIRTUALIZATION_MANAGEMENT_CDW11_NR 0xffff

/**
 * The values of Resource Type (Base 2.4, 5.3.6, Figure 587, page 530, PDF page 559)
 */
enum xnvme_spec_admin_virtualization_management_cdw10_rt {
	XNVME_SPEC_ADMIN_VIRTUALIZATION_MANAGEMENT_CDW10_RT_VQ_RESOURCES = 0x0, ///< VQ Resources (Base 2.4, 5.3.6, Figure 587, page 530, PDF page 559)
	XNVME_SPEC_ADMIN_VIRTUALIZATION_MANAGEMENT_CDW10_RT_VI_RESOURCES = 0x1, ///< VI Resources (Base 2.4, 5.3.6, Figure 587, page 530, PDF page 559)
};

/**
 * The values of Action (Base 2.4, 5.3.6, Figure 587, page 530, PDF page 559)
 */
enum xnvme_spec_admin_virtualization_management_cdw10_act {
	XNVME_SPEC_ADMIN_VIRTUALIZATION_MANAGEMENT_CDW10_ACT_PRIMARY_CONTROLLER_FLEXIBLE_ALLOCATION = 0x1, ///< Primary Controller Flexible Allocation (Base 2.4, 5.3.6, Figure 587, page 530, PDF page 559)
	XNVME_SPEC_ADMIN_VIRTUALIZATION_MANAGEMENT_CDW10_ACT_SECONDARY_CONTROLLER_OFFLINE = 0x7, ///< Secondary Controller Offline (Base 2.4, 5.3.6, Figure 587, page 530, PDF page 559)
	XNVME_SPEC_ADMIN_VIRTUALIZATION_MANAGEMENT_CDW10_ACT_SECONDARY_CONTROLLER_ASSIGN = 0x8, ///< Secondary Controller Assign (Base 2.4, 5.3.6, Figure 587, page 530, PDF page 559)
	XNVME_SPEC_ADMIN_VIRTUALIZATION_MANAGEMENT_CDW10_ACT_SECONDARY_CONTROLLER_ONLINE = 0x9, ///< Secondary Controller Online (Base 2.4, 5.3.6, Figure 587, page 530, PDF page 559)
};

/*
 * The fields of NVMe-MI Send, by the dwords of the command
 */

/*
 * The fields of NVMe-MI Receive, by the dwords of the command
 */

/*
 * The fields of Capacity Management, by the dwords of the command
 */
/** Element Identifier (Base 2.4, 5.2.3, Figure 162, page 191, PDF page 217); bits 31:16 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_CAPACITY_MANAGEMENT_CDW10_ELID 0xffff0000
/** Operation (Base 2.4, 5.2.3, Figure 162, page 191, PDF page 217); bits 3:0 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_CAPACITY_MANAGEMENT_CDW10_OPER 0xf
/** Capacity Lower (Base 2.4, 5.2.3, Figure 163, page 192, PDF page 218); bits 31:0 of Command Dword 11 */
#define XNVME_SPEC_ADMIN_CAPACITY_MANAGEMENT_CDW11_CAPL 0xffffffff
/** Capacity Upper (Base 2.4, 5.2.3, Figure 164, page 192, PDF page 218); bits 31:0 of Command Dword 12 */
#define XNVME_SPEC_ADMIN_CAPACITY_MANAGEMENT_CDW12_CAPU 0xffffffff

/**
 * The values of Operation (Base 2.4, 5.2.3, Figure 162, page 191, PDF page 217)
 */
enum xnvme_spec_admin_capacity_management_cdw10_oper {
	XNVME_SPEC_ADMIN_CAPACITY_MANAGEMENT_CDW10_OPER_SELECT_CAPACITY_CONFIGURATION = 0x0, ///< Select Capacity Configuration (Base 2.4, 5.2.3, Figure 162, page 191, PDF page 217)
	XNVME_SPEC_ADMIN_CAPACITY_MANAGEMENT_CDW10_OPER_CREATE_ENDURANCE_GROUP = 0x1, ///< Create Endurance Group (Base 2.4, 5.2.3, Figure 162, page 191, PDF page 217)
	XNVME_SPEC_ADMIN_CAPACITY_MANAGEMENT_CDW10_OPER_DELETE_ENDURANCE_GROUP = 0x2, ///< Delete Endurance Group (Base 2.4, 5.2.3, Figure 162, page 191, PDF page 217)
	XNVME_SPEC_ADMIN_CAPACITY_MANAGEMENT_CDW10_OPER_CREATE_NVM_SET = 0x3, ///< Create NVM Set (Base 2.4, 5.2.3, Figure 162, page 191, PDF page 217)
	XNVME_SPEC_ADMIN_CAPACITY_MANAGEMENT_CDW10_OPER_DELETE_NVM_SET = 0x4, ///< Delete NVM Set (Base 2.4, 5.2.3, Figure 162, page 191, PDF page 217)
	XNVME_SPEC_ADMIN_CAPACITY_MANAGEMENT_CDW10_OPER_RESTORE_DEFAULT_CAPACITY_MANAGEMENT = 0x5, ///< Restore Default Capacity Management (Base 2.4, 5.2.3, Figure 162, page 191, PDF page 217)
};

/*
 * The fields of Discovery Information Management, by the dwords of the command
 */
/** Task (Base 2.4, 5.4.2, Figure 597, page 536, PDF page 565); bits 3:0 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_DISCOVERY_INFORMATION_MANAGEMENT_CDW10_TAS 0xf

/**
 * The values of Task (Base 2.4, 5.4.2, Figure 597, page 536, PDF page 565)
 */
enum xnvme_spec_admin_discovery_information_management_cdw10_tas {
	XNVME_SPEC_ADMIN_DISCOVERY_INFORMATION_MANAGEMENT_CDW10_TAS_REGISTER = 0x0, ///< Register (Base 2.4, 5.4.2, Figure 597, page 536, PDF page 565)
	XNVME_SPEC_ADMIN_DISCOVERY_INFORMATION_MANAGEMENT_CDW10_TAS_DE_REGISTER = 0x1, ///< De-Register (Base 2.4, 5.4.2, Figure 597, page 536, PDF page 565)
	XNVME_SPEC_ADMIN_DISCOVERY_INFORMATION_MANAGEMENT_CDW10_TAS_UPDATE = 0x2, ///< Update (Base 2.4, 5.4.2, Figure 597, page 536, PDF page 565)
};

/*
 * The fields of Fabric Zoning Receive, by the dwords of the command
 */
/** Zoning Data Key (Base 2.4, 5.4.4, Figure 607, page 542, PDF page 572); bits 31:0 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_FABRIC_ZONING_RECEIVE_CDW10_ZDK 0xffffffff
/** Zoning Data Offset (Base 2.4, 5.4.4, Figure 608, page 543, PDF page 572); bits 31:0 of Command Dword 11 */
#define XNVME_SPEC_ADMIN_FABRIC_ZONING_RECEIVE_CDW11_ZDO 0xffffffff
/** ZDK Context (Base 2.4, 5.4.4, Figure 609, page 543, PDF page 572); bit 28 of Command Dword 12 */
#define XNVME_SPEC_ADMIN_FABRIC_ZONING_RECEIVE_CDW12_ZDKC 0x10000000
/** Number of Dwords (Base 2.4, 5.4.4, Figure 609, page 543, PDF page 572); bits 27:0 of Command Dword 12 */
#define XNVME_SPEC_ADMIN_FABRIC_ZONING_RECEIVE_CDW12_NUMD 0xfffffff

/*
 * The fields of Lockdown, by the dwords of the command
 */
/** Controller Select (Base 2.4, 5.2.16, Figure 365, page 403, PDF page 431); bits 19:16 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_LOCKDOWN_CDW10_CSEL 0xf0000
/** Opcode or Feature Identifier (Base 2.4, 5.2.16, Figure 365, page 403, PDF page 431); bits 15:8 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_LOCKDOWN_CDW10_OFI 0xff00
/** Interface (Base 2.4, 5.2.16, Figure 365, page 403, PDF page 431); bits 6:5 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_LOCKDOWN_CDW10_IFC 0x60
/** Prohibit (Base 2.4, 5.2.16, Figure 365, page 403, PDF page 431); bit 4 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_LOCKDOWN_CDW10_PRHBT 0x10
/** Scope (Base 2.4, 5.2.16, Figure 365, page 403, PDF page 431); bits 3:0 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_LOCKDOWN_CDW10_SCP 0xf
/** Controller Select Specific (Base 2.4, 5.2.16, Figure 366, page 405, PDF page 433); bits 31:16 of Command Dword 14 */
#define XNVME_SPEC_ADMIN_LOCKDOWN_CDW14_CSS 0xffff0000
/** UUID Index (Base 2.4, 5.2.16, Figure 366, page 405, PDF page 433); bits 6:0 of Command Dword 14 */
#define XNVME_SPEC_ADMIN_LOCKDOWN_CDW14_UIDX 0x7f

/**
 * The values of Controller Select (Base 2.4, 5.2.16, Figure 365, page 403, PDF page 431)
 */
enum xnvme_spec_admin_lockdown_cdw10_csel {
	XNVME_SPEC_ADMIN_LOCKDOWN_CDW10_CSEL_NVM_SUBSYSTEM = 0x0, ///< NVM Subsystem (Base 2.4, 5.2.16, Figure 365, page 403, PDF page 431)
	XNVME_SPEC_ADMIN_LOCKDOWN_CDW10_CSEL_SPECIFIC_CONTROLLER = 0x1, ///< Specific Controller (Base 2.4, 5.2.16, Figure 365, page 403, PDF page 431)
	XNVME_SPEC_ADMIN_LOCKDOWN_CDW10_CSEL_SECONDARY_CONTROLLERS_FOR_A_SPECIFIC_PRIMARY_CONTROLLER = 0x2, ///< Secondary Controllers for a Specific Primary Controller (Base 2.4, 5.2.16, Figure 365, page 403, PDF page 431)
	XNVME_SPEC_ADMIN_LOCKDOWN_CDW10_CSEL_VENDOR_SPECIFIC = 0xf, ///< Vendor Specific (Base 2.4, 5.2.16, Figure 365, page 403, PDF page 431)
};

/**
 * The values of Interface (Base 2.4, 5.2.16, Figure 365, page 403, PDF page 431)
 */
enum xnvme_spec_admin_lockdown_cdw10_ifc {
	XNVME_SPEC_ADMIN_LOCKDOWN_CDW10_IFC_ADMIN_SUBMISSION_QUEUE = 0x0, ///< Admin Submission Queue (Base 2.4, 5.2.16, Figure 365, page 403, PDF page 431)
	XNVME_SPEC_ADMIN_LOCKDOWN_CDW10_IFC_OUT_OF_BAND_ON_A_MANAGEMENT_ENDPOINT = 0x2, ///< Out-of-band on a Management Endpoint (Base 2.4, 5.2.16, Figure 365, page 403, PDF page 431)
};

/**
 * The values of Scope (Base 2.4, 5.2.16, Figure 365, page 403, PDF page 431)
 */
enum xnvme_spec_admin_lockdown_cdw10_scp {
	XNVME_SPEC_ADMIN_LOCKDOWN_CDW10_SCP_PCIE_COMMAND_SET_OPCODE = 0x4, ///< PCIe Command Set opcode (Base 2.4, 5.2.16, Figure 365, page 403, PDF page 431)
};

/*
 * The fields of Fabric Zoning Lookup, by the dwords of the command
 */

/*
 * The fields of Clear Exported NVM Resource Configuration, by the dwords of the command
 */

/*
 * The fields of Fabric Zoning Send, by the dwords of the command
 */
/** Zoning Data Key (Base 2.4, 5.4.5, Figure 613, page 544, PDF page 573); bits 31:0 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_FABRIC_ZONING_SEND_CDW10_ZDK 0xffffffff
/** Zoning Data Offset (Base 2.4, 5.4.5, Figure 614, page 544, PDF page 573); bits 31:0 of Command Dword 11 */
#define XNVME_SPEC_ADMIN_FABRIC_ZONING_SEND_CDW11_ZDO 0xffffffff
/** Last Fragment (Base 2.4, 5.4.5, Figure 615, page 544, PDF page 573); bit 31 of Command Dword 12 */
#define XNVME_SPEC_ADMIN_FABRIC_ZONING_SEND_CDW12_LF 0x80000000
/** ZDK Context (Base 2.4, 5.4.5, Figure 615, page 544, PDF page 573); bit 28 of Command Dword 12 */
#define XNVME_SPEC_ADMIN_FABRIC_ZONING_SEND_CDW12_ZDKC 0x10000000
/** Number of Dwords (Base 2.4, 5.4.5, Figure 615, page 544, PDF page 573); bits 27:0 of Command Dword 12 */
#define XNVME_SPEC_ADMIN_FABRIC_ZONING_SEND_CDW12_NUMD 0xfffffff

/*
 * The fields of Manage Exported NVM Subsystem Receive, by the dwords of the command
 */
/** Management Operation Specific Extended (Base 2.4, 5.2.18, Figure 375, page 410, PDF page 438); bits 31:16 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_MANAGE_EXPORTED_NVM_SUBSYSTEM_RECEIVE_CDW10_MOSE 0xffff0000
/** Management Operation Specific (Base 2.4, 5.2.18, Figure 375, page 410, PDF page 438); bits 15:8 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_MANAGE_EXPORTED_NVM_SUBSYSTEM_RECEIVE_CDW10_MOS 0xff00
/** Select (Base 2.4, 5.2.18, Figure 375, page 410, PDF page 438); bits 7:0 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_MANAGE_EXPORTED_NVM_SUBSYSTEM_RECEIVE_CDW10_SEL 0xff
/** Offset Lower (Base 2.4, 5.2.18, Figure 376, page 410, PDF page 438); bits 31:0 of Command Dword 12 */
#define XNVME_SPEC_ADMIN_MANAGE_EXPORTED_NVM_SUBSYSTEM_RECEIVE_CDW12_OL 0xffffffff
/** Offset Upper (Base 2.4, 5.2.18, Figure 377, page 411, PDF page 439); bits 31:0 of Command Dword 13 */
#define XNVME_SPEC_ADMIN_MANAGE_EXPORTED_NVM_SUBSYSTEM_RECEIVE_CDW13_OU 0xffffffff
/** Management Operation Specific Identifier (Base 2.4, 5.2.18, Figure 378, page 411, PDF page 439); bits 31:16 of Command Dword 14 */
#define XNVME_SPEC_ADMIN_MANAGE_EXPORTED_NVM_SUBSYSTEM_RECEIVE_CDW14_MOSI 0xffff0000
/** Number of Dwords (Base 2.4, 5.2.18, Figure 379, page 411, PDF page 439); bits 31:0 of Command Dword 15 */
#define XNVME_SPEC_ADMIN_MANAGE_EXPORTED_NVM_SUBSYSTEM_RECEIVE_CDW15_NUMDL 0xffffffff

/**
 * The values of Select (Base 2.4, 5.2.18, Figure 375, page 410, PDF page 438)
 */
enum xnvme_spec_admin_manage_exported_nvm_subsystem_receive_cdw10_sel {
	XNVME_SPEC_ADMIN_MANAGE_EXPORTED_NVM_SUBSYSTEM_RECEIVE_CDW10_SEL_CREATE_EXPORTED_NVM_SUBSYSTEM = 0x0, ///< Create Exported NVM Subsystem (Base 2.4, 5.2.18, Figure 375, page 410, PDF page 438)
	XNVME_SPEC_ADMIN_MANAGE_EXPORTED_NVM_SUBSYSTEM_RECEIVE_CDW10_SEL_LIST_EXPORTED_NVM_SUBSYSTEMS = 0x1, ///< List Exported NVM Subsystems (Base 2.4, 5.2.18, Figure 375, page 410, PDF page 438)
	XNVME_SPEC_ADMIN_MANAGE_EXPORTED_NVM_SUBSYSTEM_RECEIVE_CDW10_SEL_LIST_EXPORTED_NAMESPACES = 0x2, ///< List Exported Namespaces (Base 2.4, 5.2.18, Figure 375, page 410, PDF page 438)
	XNVME_SPEC_ADMIN_MANAGE_EXPORTED_NVM_SUBSYSTEM_RECEIVE_CDW10_SEL_LIST_EXPORTED_CONTROLLERS1 = 0x3, ///< List Exported Controllers1 (Base 2.4, 5.2.18, Figure 375, page 410, PDF page 438)
	XNVME_SPEC_ADMIN_MANAGE_EXPORTED_NVM_SUBSYSTEM_RECEIVE_CDW10_SEL_LIST_EXPORTED_PORTS2 = 0x4, ///< List Exported Ports2 (Base 2.4, 5.2.18, Figure 375, page 410, PDF page 438)
	XNVME_SPEC_ADMIN_MANAGE_EXPORTED_NVM_SUBSYSTEM_RECEIVE_CDW10_SEL_GET_EXPORTED_NVM_SUBSYSTEM_CONFIGURATION_STATE = 0x5, ///< Get Exported NVM Subsystem Configuration State (Base 2.4, 5.2.18, Figure 375, page 410, PDF page 438)
};

/*
 * The fields of Manage Exported NVM Subsystem Send, by the dwords of the command
 */
/** Management Operation Specific Extended (Base 2.4, 5.2.19, Figure 397, page 418, PDF page 446); bits 31:16 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_MANAGE_EXPORTED_NVM_SUBSYSTEM_SEND_CDW10_MOSE 0xffff0000
/** Management Operation Specific (Base 2.4, 5.2.19, Figure 397, page 418, PDF page 446); bits 15:8 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_MANAGE_EXPORTED_NVM_SUBSYSTEM_SEND_CDW10_MOS 0xff00
/** Select (Base 2.4, 5.2.19, Figure 397, page 418, PDF page 446); bits 7:0 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_MANAGE_EXPORTED_NVM_SUBSYSTEM_SEND_CDW10_SEL 0xff
/** Exported NVM Subsystem Identifier (Base 2.4, 5.2.19, Figure 398, page 418, PDF page 447); bits 31:16 of Command Dword 14 */
#define XNVME_SPEC_ADMIN_MANAGE_EXPORTED_NVM_SUBSYSTEM_SEND_CDW14_ESUBID 0xffff0000
/** Exported NVM Subsystem Identifier Valid (Base 2.4, 5.2.19, Figure 398, page 418, PDF page 447); bit 7 of Command Dword 14 */
#define XNVME_SPEC_ADMIN_MANAGE_EXPORTED_NVM_SUBSYSTEM_SEND_CDW14_ESUBIDV 0x80
/** Number of Dwords (Base 2.4, 5.2.19.1.6, Figure 406, page 424, PDF page 452); bits 31:0 of Command Dword 15 */
#define XNVME_SPEC_ADMIN_MANAGE_EXPORTED_NVM_SUBSYSTEM_SEND_CDW15_NUMD 0xffffffff

/**
 * The values of Select (Base 2.4, 5.2.19, Figure 397, page 418, PDF page 446)
 */
enum xnvme_spec_admin_manage_exported_nvm_subsystem_send_cdw10_sel {
	XNVME_SPEC_ADMIN_MANAGE_EXPORTED_NVM_SUBSYSTEM_SEND_CDW10_SEL_DELETE = 0x1, ///< Delete (Base 2.4, 5.2.19, Figure 397, page 418, PDF page 446)
	XNVME_SPEC_ADMIN_MANAGE_EXPORTED_NVM_SUBSYSTEM_SEND_CDW10_SEL_CHANGE_ACCESS_MODE1 = 0x2, ///< Change Access Mode1 (Base 2.4, 5.2.19, Figure 397, page 418, PDF page 446)
	XNVME_SPEC_ADMIN_MANAGE_EXPORTED_NVM_SUBSYSTEM_SEND_CDW10_SEL_GRANT_HOST_ACCESS1 = 0x3, ///< Grant Host Access1 (Base 2.4, 5.2.19, Figure 397, page 418, PDF page 446)
	XNVME_SPEC_ADMIN_MANAGE_EXPORTED_NVM_SUBSYSTEM_SEND_CDW10_SEL_REVOKE_HOST_ACCESS1 = 0x4, ///< Revoke Host Access1 (Base 2.4, 5.2.19, Figure 397, page 418, PDF page 446)
	XNVME_SPEC_ADMIN_MANAGE_EXPORTED_NVM_SUBSYSTEM_SEND_CDW10_SEL_ASSOCIATE_CONTROLLERS2 = 0x5, ///< Associate Controllers2 (Base 2.4, 5.2.19, Figure 397, page 418, PDF page 446)
	XNVME_SPEC_ADMIN_MANAGE_EXPORTED_NVM_SUBSYSTEM_SEND_CDW10_SEL_SET_EXPORTED_CONFIGURATION_STATE = 0x6, ///< Set Exported Configuration State (Base 2.4, 5.2.19, Figure 397, page 418, PDF page 446)
};

/*
 * The fields of Manage Exported Namespace, by the dwords of the command
 */
/** Management Operation Specific (Base 2.4, 5.2.17, Figure 369, page 406, PDF page 434); bits 15:8 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_MANAGE_EXPORTED_NAMESPACE_CDW10_MOS 0xff00
/** Select (Base 2.4, 5.2.17, Figure 369, page 406, PDF page 434); bits 7:0 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_MANAGE_EXPORTED_NAMESPACE_CDW10_SEL 0xff
/** Exported NVM Subsystem Identifier (Base 2.4, 5.2.17, Figure 370, page 407, PDF page 435); bits 31:16 of Command Dword 14 */
#define XNVME_SPEC_ADMIN_MANAGE_EXPORTED_NAMESPACE_CDW14_ESUBID 0xffff0000
/** Exported NVM Subsystem Identifier Valid (Base 2.4, 5.2.17, Figure 370, page 407, PDF page 435); bit 7 of Command Dword 14 */
#define XNVME_SPEC_ADMIN_MANAGE_EXPORTED_NAMESPACE_CDW14_ESUBIDV 0x80

/**
 * The values of Select (Base 2.4, 5.2.17, Figure 369, page 406, PDF page 434)
 */
enum xnvme_spec_admin_manage_exported_namespace_cdw10_sel {
	XNVME_SPEC_ADMIN_MANAGE_EXPORTED_NAMESPACE_CDW10_SEL_ASSOCIATE_NAMESPACE = 0x1, ///< Associate Namespace (Base 2.4, 5.2.17, Figure 369, page 406, PDF page 434)
	XNVME_SPEC_ADMIN_MANAGE_EXPORTED_NAMESPACE_CDW10_SEL_DISASSOCIATE_NAMESPACE = 0x2, ///< Disassociate Namespace (Base 2.4, 5.2.17, Figure 369, page 406, PDF page 434)
};

/*
 * The fields of Manage Exported Port, by the dwords of the command
 */

/*
 * The fields of Cross-Controller Reset, by the dwords of the command
 */
/** Controller Instance Uniquifer (Base 2.4, 5.4.1, Figure 591, page 533, PDF page 562); bits 23:16 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_CROSS_CONTROLLER_RESET_CDW10_CIU 0xff0000
/** Impacted Controller ID (Base 2.4, 5.4.1, Figure 591, page 533, PDF page 562); bits 15:0 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_CROSS_CONTROLLER_RESET_CDW10_ICID 0xffff
/** Controller Instance Random Number Lower (Base 2.4, 5.4.1, Figure 592, page 533, PDF page 562); bits 31:0 of Command Dword 12 */
#define XNVME_SPEC_ADMIN_CROSS_CONTROLLER_RESET_CDW12_CIRNL 0xffffffff
/** Controller Instance Random Number Upper (Base 2.4, 5.4.1, Figure 593, page 533, PDF page 562); bits 31:0 of Command Dword 13 */
#define XNVME_SPEC_ADMIN_CROSS_CONTROLLER_RESET_CDW13_CIRNU 0xffffffff

/*
 * The fields of Send Discovery Log Page, by the dwords of the command
 */
/** Requested Log Page Status (Base 2.4, 5.4.7, Figure 624, page 547, PDF page 576); bits 31:30 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_SEND_DISCOVERY_LOG_PAGE_CDW10_RLPS 0xc0000000
/** Status Code Type (Base 2.4, 5.4.7, Figure 624, page 547, PDF page 576); bits 27:25 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_SEND_DISCOVERY_LOG_PAGE_CDW10_SCT 0xe000000
/** Status Code (Base 2.4, 5.4.7, Figure 624, page 547, PDF page 576); bits 24:17 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_SEND_DISCOVERY_LOG_PAGE_CDW10_SC 0x1fe0000
/** Transferred Log Specific Parameter (Base 2.4, 5.4.7, Figure 624, page 547, PDF page 576); bits 14:8 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_SEND_DISCOVERY_LOG_PAGE_CDW10_TLSP 0x7f00
/** Transferred Log Page Identifier (Base 2.4, 5.4.7, Figure 624, page 547, PDF page 576); bits 7:0 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_SEND_DISCOVERY_LOG_PAGE_CDW10_TLID 0xff
/** Number of Dwords (Base 2.4, 5.4.7, Figure 625, page 547, PDF page 577); bits 31:0 of Command Dword 11 */
#define XNVME_SPEC_ADMIN_SEND_DISCOVERY_LOG_PAGE_CDW11_NDWS 0xffffffff
/** Transferred Log Page Offset Lower (Base 2.4, 5.4.7, Figure 626, page 547, PDF page 577); bits 31:0 of Command Dword 12 */
#define XNVME_SPEC_ADMIN_SEND_DISCOVERY_LOG_PAGE_CDW12_TLPOL 0xffffffff
/** Transferred Log Page Offset Upper (Base 2.4, 5.4.7, Figure 627, page 548, PDF page 577); bits 31:0 of Command Dword 13 */
#define XNVME_SPEC_ADMIN_SEND_DISCOVERY_LOG_PAGE_CDW13_TLPOU 0xffffffff

/**
 * The values of Requested Log Page Status (Base 2.4, 5.4.7, Figure 624, page 547, PDF page 576)
 */
enum xnvme_spec_admin_send_discovery_log_page_cdw10_rlps {
	XNVME_SPEC_ADMIN_SEND_DISCOVERY_LOG_PAGE_CDW10_RLPS_VALID_LOG_PAGE = 0x0, ///< Valid Log Page (Base 2.4, 5.4.7, Figure 624, page 547, PDF page 576)
	XNVME_SPEC_ADMIN_SEND_DISCOVERY_LOG_PAGE_CDW10_RLPS_INVALID_LOG_PAGE = 0x1, ///< Invalid Log Page (Base 2.4, 5.4.7, Figure 624, page 547, PDF page 576)
	XNVME_SPEC_ADMIN_SEND_DISCOVERY_LOG_PAGE_CDW10_RLPS_NOT_ALLOWED_LOG_PAGE = 0x2, ///< Not Allowed Log Page (Base 2.4, 5.4.7, Figure 624, page 547, PDF page 576)
	XNVME_SPEC_ADMIN_SEND_DISCOVERY_LOG_PAGE_CDW10_RLPS_NOT_SUCCESSFUL = 0x3, ///< Not Successful (Base 2.4, 5.4.7, Figure 624, page 547, PDF page 576)
};

/*
 * The fields of Track Send, by the dwords of the command
 */
/** Management Operation Specific (Base 2.4, 5.2.32, Figure 561, page 519, PDF page 549); bits 31:16 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_TRACK_SEND_CDW10_MOS 0xffff0000
/** Select (Base 2.4, 5.2.32, Figure 561, page 519, PDF page 549); bits 7:0 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_TRACK_SEND_CDW10_SEL 0xff

/**
 * The values of Select (Base 2.4, 5.2.32, Figure 561, page 519, PDF page 549)
 */
enum xnvme_spec_admin_track_send_cdw10_sel {
	XNVME_SPEC_ADMIN_TRACK_SEND_CDW10_SEL_LOG_USER_DATA_CHANGES = 0x0, ///< Log User Data Changes (Base 2.4, 5.2.32, Figure 561, page 519, PDF page 549)
	XNVME_SPEC_ADMIN_TRACK_SEND_CDW10_SEL_TRACK_MEMORY_CHANGES = 0x1, ///< Track Memory Changes (Base 2.4, 5.2.32, Figure 561, page 519, PDF page 549)
};

/*
 * The fields of Track Receive, by the dwords of the command
 */
/** Select (Base 2.4, 5.2.31, Figure 555, page 517, PDF page 546); bits 7:0 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_TRACK_RECEIVE_CDW10_SEL 0xff
/** Number of Dwords (Base 2.4, 5.2.31, Figure 556, page 517, PDF page 546); bits 31:0 of Command Dword 12 */
#define XNVME_SPEC_ADMIN_TRACK_RECEIVE_CDW12_NUMDL 0xffffffff

/**
 * The values of Select (Base 2.4, 5.2.31, Figure 555, page 517, PDF page 546)
 */
enum xnvme_spec_admin_track_receive_cdw10_sel {
	XNVME_SPEC_ADMIN_TRACK_RECEIVE_CDW10_SEL_TRACKED_MEMORY_CHANGES = 0x0, ///< Tracked Memory Changes (Base 2.4, 5.2.31, Figure 555, page 517, PDF page 546)
};

/*
 * The fields of Migration Send, by the dwords of the command
 */
/** Management Operation Specific (Base 2.4, 5.2.21, Figure 421, page 430, PDF page 458); bits 31:16 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_MIGRATION_SEND_CDW10_MOS 0xffff0000
/** Select (Base 2.4, 5.2.21, Figure 421, page 430, PDF page 458); bits 7:0 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_MIGRATION_SEND_CDW10_SEL 0xff
/** UUID Index (Base 2.4, 5.2.21, Figure 422, page 430, PDF page 458); bits 6:0 of Command Dword 14 */
#define XNVME_SPEC_ADMIN_MIGRATION_SEND_CDW14_UIDX 0x7f

/**
 * The values of Select (Base 2.4, 5.2.21, Figure 421, page 430, PDF page 458)
 */
enum xnvme_spec_admin_migration_send_cdw10_sel {
	XNVME_SPEC_ADMIN_MIGRATION_SEND_CDW10_SEL_SUSPEND = 0x0, ///< Suspend (Base 2.4, 5.2.21, Figure 421, page 430, PDF page 458)
	XNVME_SPEC_ADMIN_MIGRATION_SEND_CDW10_SEL_RESUME = 0x1, ///< Resume (Base 2.4, 5.2.21, Figure 421, page 430, PDF page 458)
	XNVME_SPEC_ADMIN_MIGRATION_SEND_CDW10_SEL_SET_CONTROLLER_STATE = 0x2, ///< Set Controller State (Base 2.4, 5.2.21, Figure 421, page 430, PDF page 458)
	XNVME_SPEC_ADMIN_MIGRATION_SEND_CDW10_SEL_SET_EXPORTED_NVM_SUBSYSTEM_STATE = 0x3, ///< Set Exported NVM Subsystem State (Base 2.4, 5.2.21, Figure 421, page 430, PDF page 458)
};

/*
 * The fields of Migration Receive, by the dwords of the command
 */
/** Management Operation Specific (Base 2.4, 5.2.20, Figure 409, page 425, PDF page 453); bits 31:16 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_MIGRATION_RECEIVE_CDW10_MOS 0xffff0000
/** Select (Base 2.4, 5.2.20, Figure 409, page 425, PDF page 453); bits 7:0 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_MIGRATION_RECEIVE_CDW10_SEL 0xff
/** Offset Lower (Base 2.4, 5.2.20, Figure 410, page 425, PDF page 453); bits 31:0 of Command Dword 12 */
#define XNVME_SPEC_ADMIN_MIGRATION_RECEIVE_CDW12_OL 0xffffffff
/** Offset Upper (Base 2.4, 5.2.20, Figure 411, page 425, PDF page 453); bits 31:0 of Command Dword 13 */
#define XNVME_SPEC_ADMIN_MIGRATION_RECEIVE_CDW13_OU 0xffffffff
/** UUID Index (Base 2.4, 5.2.20, Figure 412, page 426, PDF page 454); bits 6:0 of Command Dword 14 */
#define XNVME_SPEC_ADMIN_MIGRATION_RECEIVE_CDW14_UIDX 0x7f
/** Number of Dwords (Base 2.4, 5.2.20, Figure 413, page 426, PDF page 454); bits 31:0 of Command Dword 15 */
#define XNVME_SPEC_ADMIN_MIGRATION_RECEIVE_CDW15_NUMDL 0xffffffff

/**
 * The values of Select (Base 2.4, 5.2.20, Figure 409, page 425, PDF page 453)
 */
enum xnvme_spec_admin_migration_receive_cdw10_sel {
	XNVME_SPEC_ADMIN_MIGRATION_RECEIVE_CDW10_SEL_GET_CONTROLLER_STATE = 0x0, ///< Get Controller State (Base 2.4, 5.2.20, Figure 409, page 425, PDF page 453)
	XNVME_SPEC_ADMIN_MIGRATION_RECEIVE_CDW10_SEL_GET_EXPORTED_NVM_SUBSYSTEM_STATE = 0x1, ///< Get Exported NVM Subsystem State (Base 2.4, 5.2.20, Figure 409, page 425, PDF page 453)
};

/*
 * The fields of Controller Data Queue, by the dwords of the command
 */
/** Management Operation Specific (Base 2.4, 5.2.5, Figure 167, page 196, PDF page 222); bits 31:16 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_CONTROLLER_DATA_QUEUE_CDW10_MOS 0xffff0000
/** Select (Base 2.4, 5.2.5, Figure 167, page 196, PDF page 222); bits 7:0 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_CONTROLLER_DATA_QUEUE_CDW10_SEL 0xff
/** Enable Tail Pointer Trigger (Base 2.4, 5.2.30.1.24, Figure 508, page 482, PDF page 510); bit 31 of Command Dword 11 */
#define XNVME_SPEC_ADMIN_CONTROLLER_DATA_QUEUE_CDW11_ETPT 0x80000000
/** Controller Data Queue Identifier (Base 2.4, 5.2.30.1.24, Figure 508, page 482, PDF page 510); bits 15:0 of Command Dword 11 */
#define XNVME_SPEC_ADMIN_CONTROLLER_DATA_QUEUE_CDW11_CDQID 0xffff
/** Head Pointer (Base 2.4, 5.2.30.1.24, Figure 509, page 482, PDF page 510); bits 31:0 of Command Dword 12 */
#define XNVME_SPEC_ADMIN_CONTROLLER_DATA_QUEUE_CDW12_HP 0xffffffff
/** Tail Pointer Trigger (Base 2.4, 5.2.30.1.24, Figure 510, page 482, PDF page 510); bits 31:0 of Command Dword 13 */
#define XNVME_SPEC_ADMIN_CONTROLLER_DATA_QUEUE_CDW13_TPT 0xffffffff

/**
 * The values of Select (Base 2.4, 5.2.5, Figure 167, page 196, PDF page 222)
 */
enum xnvme_spec_admin_controller_data_queue_cdw10_sel {
	XNVME_SPEC_ADMIN_CONTROLLER_DATA_QUEUE_CDW10_SEL_CREATE_CONTROLLER_DATA_QUEUE = 0x0, ///< Create Controller Data Queue (Base 2.4, 5.2.5, Figure 167, page 196, PDF page 222)
	XNVME_SPEC_ADMIN_CONTROLLER_DATA_QUEUE_CDW10_SEL_DELETE_CONTROLLER_DATA_QUEUE = 0x1, ///< Delete Controller Data Queue (Base 2.4, 5.2.5, Figure 167, page 196, PDF page 222)
};

/*
 * The fields of Doorbell Buffer Config, by the dwords of the command
 */

/*
 * The fields of Format NVM, by the dwords of the command
 */
/** LBA Format Upper (Base 2.4, 5.2.11, Figure 195, page 208, PDF page 234); bits 13:12 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_FORMAT_NVM_CDW10_LBAFU 0x3000
/** Secure Erase Settings (Base 2.4, 5.2.11, Figure 195, page 208, PDF page 234); bits 11:9 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_FORMAT_NVM_CDW10_SES 0xe00
/** Protection Information Location (Base 2.4, 5.2.11, Figure 195, page 208, PDF page 234); bit 8 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_FORMAT_NVM_CDW10_PIL 0x100
/** Protection Information (Base 2.4, 5.2.11, Figure 195, page 208, PDF page 234); bits 7:5 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_FORMAT_NVM_CDW10_PI 0xe0
/** Metadata Settings (Base 2.4, 5.2.11, Figure 195, page 208, PDF page 234); bit 4 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_FORMAT_NVM_CDW10_MSET 0x10
/** LBA Format Lower (Base 2.4, 5.2.11, Figure 195, page 208, PDF page 234); bits 3:0 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_FORMAT_NVM_CDW10_LBAFL 0xf

/**
 * The values of Secure Erase Settings (Base 2.4, 5.2.11, Figure 195, page 208, PDF page 234)
 */
enum xnvme_spec_admin_format_nvm_cdw10_ses {
	XNVME_SPEC_ADMIN_FORMAT_NVM_CDW10_SES_USER_DATA_ERASE = 0x1, ///< User Data Erase (Base 2.4, 5.2.11, Figure 195, page 208, PDF page 234)
	XNVME_SPEC_ADMIN_FORMAT_NVM_CDW10_SES_CRYPTOGRAPHIC_ERASE = 0x2, ///< Cryptographic Erase (Base 2.4, 5.2.11, Figure 195, page 208, PDF page 234)
};

/*
 * The fields of Security Send, by the dwords of the command
 */
/** Security Protocol (Base 2.4, 5.2.29, Figure 461, page 454, PDF page 482); bits 31:24 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_SECURITY_SEND_CDW10_SECP 0xff000000
/** SP Specific 1 (Base 2.4, 5.2.29, Figure 461, page 454, PDF page 482); bits 23:16 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_SECURITY_SEND_CDW10_SPSP1 0xff0000
/** SP Specific 0 (Base 2.4, 5.2.29, Figure 461, page 454, PDF page 482); bits 15:8 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_SECURITY_SEND_CDW10_SPSP0 0xff00
/** NVMe Security Specific Field (Base 2.4, 5.2.29, Figure 461, page 454, PDF page 482); bits 7:0 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_SECURITY_SEND_CDW10_NSSF 0xff
/** Transfer Length (Base 2.4, 5.2.29, Figure 462, page 454, PDF page 482); bits 31:0 of Command Dword 11 */
#define XNVME_SPEC_ADMIN_SECURITY_SEND_CDW11_TL 0xffffffff

/*
 * The fields of Security Receive, by the dwords of the command
 */
/** Security Protocol (Base 2.4, 5.2.28, Figure 457, page 452, PDF page 480); bits 31:24 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_SECURITY_RECEIVE_CDW10_SECP 0xff000000
/** SP Specific 1 (Base 2.4, 5.2.28, Figure 457, page 452, PDF page 480); bits 23:16 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_SECURITY_RECEIVE_CDW10_SPSP1 0xff0000
/** SP Specific 0 (Base 2.4, 5.2.28, Figure 457, page 452, PDF page 480); bits 15:8 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_SECURITY_RECEIVE_CDW10_SPSP0 0xff00
/** NVMe Security Specific Field (Base 2.4, 5.2.28, Figure 457, page 452, PDF page 480); bits 7:0 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_SECURITY_RECEIVE_CDW10_NSSF 0xff
/** Allocation Length (Base 2.4, 5.2.28, Figure 458, page 453, PDF page 481); bits 31:0 of Command Dword 11 */
#define XNVME_SPEC_ADMIN_SECURITY_RECEIVE_CDW11_AL 0xffffffff

/*
 * The fields of Sanitize, by the dwords of the command
 */
/** Purge Required (Base 2.4, 5.2.26, Figure 451, page 448, PDF page 476); bit 11 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_SANITIZE_CDW10_PREQ 0x800
/** Enter Media Verification State (Base 2.4, 5.2.26, Figure 451, page 448, PDF page 476); bit 10 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_SANITIZE_CDW10_EMVS 0x400
/** No-Deallocate After Sanitize (Base 2.4, 5.2.26, Figure 451, page 448, PDF page 476); bit 9 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_SANITIZE_CDW10_NDAS 0x200
/** Overwrite Invert Pattern Between Passes (Base 2.4, 5.2.26, Figure 451, page 448, PDF page 476); bit 8 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_SANITIZE_CDW10_OIPBP 0x100
/** Overwrite Pass Count (Base 2.4, 5.2.26, Figure 451, page 448, PDF page 476); bits 7:4 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_SANITIZE_CDW10_OWPASS 0xf0
/** Allow Unrestricted Sanitize Exit (Base 2.4, 5.2.26, Figure 451, page 448, PDF page 476); bit 3 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_SANITIZE_CDW10_AUSE 0x8
/** Sanitize Action (Base 2.4, 5.2.26, Figure 451, page 448, PDF page 476); bits 2:0 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_SANITIZE_CDW10_SANACT 0x7
/** Overwrite Pattern (Base 2.4, 5.2.26, Figure 452, page 449, PDF page 477); bits 31:0 of Command Dword 11 */
#define XNVME_SPEC_ADMIN_SANITIZE_CDW11_OVRPAT 0xffffffff

/**
 * The values of Sanitize Action (Base 2.4, 5.2.26, Figure 451, page 448, PDF page 476)
 */
enum xnvme_spec_admin_sanitize_cdw10_sanact {
	XNVME_SPEC_ADMIN_SANITIZE_CDW10_SANACT_EXIT_FAILURE_MODE = 0x1, ///< Exit Failure Mode (Base 2.4, 5.2.26, Figure 451, page 448, PDF page 476)
	XNVME_SPEC_ADMIN_SANITIZE_CDW10_SANACT_EXIT_MEDIA_VERIFICATION_STATE = 0x5, ///< Exit Media Verification State (Base 2.4, 5.2.26, Figure 451, page 448, PDF page 476)
};

/*
 * The fields of Load Program, by the dwords of the command
 */
/** Program Identifier Type (Computational Programs 1.3, 4.2.1, Figure 40, page 36, PDF page 36); bits 27:25 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_LOAD_PROGRAM_CDW10_PIT 0xe000000
/** Select (Computational Programs 1.3, 4.2.1, Figure 40, page 36, PDF page 36); bit 24 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_LOAD_PROGRAM_CDW10_SEL 0x1000000
/** Program Type (Computational Programs 1.3, 4.2.1, Figure 40, page 36, PDF page 36); bits 23:16 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_LOAD_PROGRAM_CDW10_PTYPE 0xff0000
/** Program Index (Computational Programs 1.3, 4.2.1, Figure 40, page 36, PDF page 36); bits 15:0 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_LOAD_PROGRAM_CDW10_PIND 0xffff
/** Program Size (Computational Programs 1.3, 4.2.1, Figure 41, page 36, PDF page 36); bits 31:0 of Command Dword 11 */
#define XNVME_SPEC_ADMIN_LOAD_PROGRAM_CDW11_PSIZE 0xffffffff
/** Number of Bytes (Computational Programs 1.3, 4.2.1, Figure 43, page 36, PDF page 36); bits 31:0 of Command Dword 14 */
#define XNVME_SPEC_ADMIN_LOAD_PROGRAM_CDW14_NUMB 0xffffffff
/** Load Offset (Computational Programs 1.3, 4.2.1, Figure 44, page 37, PDF page 37); bits 31:0 of Command Dword 15 */
#define XNVME_SPEC_ADMIN_LOAD_PROGRAM_CDW15_LOFF 0xffffffff

/**
 * The values of Program Identifier Type (Computational Programs 1.3, 4.2.1, Figure 40, page 36, PDF page 36)
 */
enum xnvme_spec_admin_load_program_cdw10_pit {
	XNVME_SPEC_ADMIN_LOAD_PROGRAM_CDW10_PIT_PID_IS_A_PROGRAM_UNIQUE_IDENTIFIER = 0x1, ///< PID is a Program Unique Identifier (Computational Programs 1.3, 4.2.1, Figure 40, page 36, PDF page 36)
};

/**
 * The values of Program Type (Computational Programs 1.3, 4.1.4.2, Figure 29, page 31, PDF page 31)
 */
enum xnvme_spec_admin_load_program_cdw10_ptype {
	XNVME_SPEC_ADMIN_LOAD_PROGRAM_CDW10_PTYPE_DEVICE_DEFINED = 0x0, ///< Device-defined (Computational Programs 1.3, 4.1.4.2, Figure 29, page 31, PDF page 31)
};

/*
 * The fields of Get LBA Status, by the dwords of the command
 */
/** Starting LBA (NVM 1.3, 4.2.1, Figure 136, page 114, PDF page 114); bits 63:0 of Command Dwords 10 and 11, as one 64-bit value */
#define XNVME_SPEC_ADMIN_GET_LBA_STATUS_CDW10_11_SLBA 0xffffffffffffffff
/** Maximum Number of Dwords (NVM 1.3, 4.2.1, Figure 137, page 114, PDF page 114); bits 31:0 of Command Dword 12 */
#define XNVME_SPEC_ADMIN_GET_LBA_STATUS_CDW12_MNDW 0xffffffff
/** Action Type (NVM 1.3, 4.2.1, Figure 138, page 114, PDF page 114); bits 31:24 of Command Dword 13 */
#define XNVME_SPEC_ADMIN_GET_LBA_STATUS_CDW13_ATYPE 0xff000000
/** Range Length (NVM 1.3, 4.2.1, Figure 138, page 114, PDF page 114); bits 15:0 of Command Dword 13 */
#define XNVME_SPEC_ADMIN_GET_LBA_STATUS_CDW13_RL 0xffff

/*
 * The fields of Program Activation Management, by the dwords of the command
 */
/** Select (Computational Programs 1.3, 4.2.3, Figure 52, page 39, PDF page 39); bits 19:16 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_PROGRAM_ACTIVATION_MANAGEMENT_CDW10_SEL 0xf0000
/** Program Index (Computational Programs 1.3, 4.2.3, Figure 52, page 39, PDF page 39); bits 15:0 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_PROGRAM_ACTIVATION_MANAGEMENT_CDW10_PIND 0xffff

/**
 * The values of Select (Computational Programs 1.3, 4.2.3, Figure 52, page 39, PDF page 39)
 */
enum xnvme_spec_admin_program_activation_management_cdw10_sel {
	XNVME_SPEC_ADMIN_PROGRAM_ACTIVATION_MANAGEMENT_CDW10_SEL_DEACTIVATE = 0x0, ///< Deactivate (Computational Programs 1.3, 4.2.3, Figure 52, page 39, PDF page 39)
	XNVME_SPEC_ADMIN_PROGRAM_ACTIVATION_MANAGEMENT_CDW10_SEL_ACTIVATE = 0x1, ///< Activate (Computational Programs 1.3, 4.2.3, Figure 52, page 39, PDF page 39)
};

/*
 * The fields of Memory Range Set Management, by the dwords of the command
 */
/** Memory Range Set ID (Computational Programs 1.3, 4.2.2, Figure 47, page 38, PDF page 38); bits 31:16 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_MEMORY_RANGE_SET_MANAGEMENT_CDW10_RSID 0xffff0000
/** Select (Computational Programs 1.3, 4.2.2, Figure 47, page 38, PDF page 38); bits 3:0 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_MEMORY_RANGE_SET_MANAGEMENT_CDW10_SEL 0xf
/** Number of Memory Ranges (Computational Programs 1.3, 4.2.2, Figure 48, page 38, PDF page 38); bits 7:0 of Command Dword 11 */
#define XNVME_SPEC_ADMIN_MEMORY_RANGE_SET_MANAGEMENT_CDW11_NUMR 0xff

/**
 * The values of Select (Computational Programs 1.3, 4.2.2, Figure 47, page 38, PDF page 38)
 */
enum xnvme_spec_admin_memory_range_set_management_cdw10_sel {
	XNVME_SPEC_ADMIN_MEMORY_RANGE_SET_MANAGEMENT_CDW10_SEL_CREATE = 0x0, ///< Create (Computational Programs 1.3, 4.2.2, Figure 47, page 38, PDF page 38)
	XNVME_SPEC_ADMIN_MEMORY_RANGE_SET_MANAGEMENT_CDW10_SEL_DELETE = 0x1, ///< Delete (Computational Programs 1.3, 4.2.2, Figure 47, page 38, PDF page 38)
};

/*
 * The fields of Sanitize Namespace, by the dwords of the command
 */
/** Enter Media Verification State (Base 2.4, 5.2.27, Figure 454, page 451, PDF page 479); bit 10 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_SANITIZE_NAMESPACE_CDW10_EMVS 0x400
/** Purge Required (Base 2.4, 5.2.27, Figure 454, page 451, PDF page 479); bit 4 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_SANITIZE_NAMESPACE_CDW10_PREQ 0x10
/** Allow Unrestricted Sanitize Exit (Base 2.4, 5.2.27, Figure 454, page 451, PDF page 479); bit 3 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_SANITIZE_NAMESPACE_CDW10_AUSE 0x8
/** Sanitize Action (Base 2.4, 5.2.27, Figure 454, page 451, PDF page 479); bits 2:0 of Command Dword 10 */
#define XNVME_SPEC_ADMIN_SANITIZE_NAMESPACE_CDW10_SANACT 0x7

/**
 * The values of Sanitize Action (Base 2.4, 5.2.27, Figure 454, page 451, PDF page 479)
 */
enum xnvme_spec_admin_sanitize_namespace_cdw10_sanact {
	XNVME_SPEC_ADMIN_SANITIZE_NAMESPACE_CDW10_SANACT_EXIT_FAILURE_MODE = 0x1, ///< Exit Failure Mode (Base 2.4, 5.2.27, Figure 454, page 451, PDF page 479)
	XNVME_SPEC_ADMIN_SANITIZE_NAMESPACE_CDW10_SANACT_EXIT_MEDIA_VERIFICATION_STATE = 0x5, ///< Exit Media Verification State (Base 2.4, 5.2.27, Figure 454, page 451, PDF page 479)
};

/*
 * The fields of Flush, by the dwords of the command
 */

/*
 * The fields of Reservation Register, by the dwords of the command
 */
/** Change Persist Through Power Loss State (Base 2.4, 7.6, Figure 663, page 569, PDF page 599); bits 31:30 of Command Dword 10 */
#define XNVME_SPEC_IO_RESERVATION_REGISTER_CDW10_CPTPL 0xc0000000
/** Dispersed Namespace Reservation Support (Base 2.4, 7.6, Figure 663, page 569, PDF page 599); bit 4 of Command Dword 10 */
#define XNVME_SPEC_IO_RESERVATION_REGISTER_CDW10_DISNSRS 0x10
/** Ignore Existing Key (Base 2.4, 7.6, Figure 663, page 569, PDF page 599); bit 3 of Command Dword 10 */
#define XNVME_SPEC_IO_RESERVATION_REGISTER_CDW10_IEKEY 0x8
/** Reservation Register Action (Base 2.4, 7.6, Figure 663, page 569, PDF page 599); bits 2:0 of Command Dword 10 */
#define XNVME_SPEC_IO_RESERVATION_REGISTER_CDW10_RREGA 0x7

/**
 * The values of Reservation Register Action (Base 2.4, 7.6, Figure 663, page 569, PDF page 599)
 */
enum xnvme_spec_io_reservation_register_cdw10_rrega {
	XNVME_SPEC_IO_RESERVATION_REGISTER_CDW10_RREGA_REGISTER_RESERVATION_KEY = 0x0, ///< Register Reservation Key (Base 2.4, 7.6, Figure 663, page 569, PDF page 599)
	XNVME_SPEC_IO_RESERVATION_REGISTER_CDW10_RREGA_UNREGISTER_RESERVATION_KEY = 0x1, ///< Unregister Reservation Key (Base 2.4, 7.6, Figure 663, page 569, PDF page 599)
	XNVME_SPEC_IO_RESERVATION_REGISTER_CDW10_RREGA_REPLACE_RESERVATION_KEY = 0x2, ///< Replace Reservation Key (Base 2.4, 7.6, Figure 663, page 569, PDF page 599)
};

/*
 * The fields of Reservation Report, by the dwords of the command
 */
/** Number of Dwords (Base 2.4, 7.8, Figure 669, page 571, PDF page 601); bits 31:0 of Command Dword 10 */
#define XNVME_SPEC_IO_RESERVATION_REPORT_CDW10_NUMD 0xffffffff
/** Dispersed Namespace Reservation Support (Base 2.4, 7.8, Figure 670, page 571, PDF page 601); bit 1 of Command Dword 11 */
#define XNVME_SPEC_IO_RESERVATION_REPORT_CDW11_DISNSRS 0x2
/** Extended Data Structure (Base 2.4, 7.8, Figure 670, page 571, PDF page 601); bit 0 of Command Dword 11 */
#define XNVME_SPEC_IO_RESERVATION_REPORT_CDW11_EDS 0x1

/*
 * The fields of Reservation Acquire, by the dwords of the command
 */
/** Reservation Type (Base 2.4, 7.5, Figure 659, page 567, PDF page 597); bits 15:8 of Command Dword 10 */
#define XNVME_SPEC_IO_RESERVATION_ACQUIRE_CDW10_RTYPE 0xff00
/** Dispersed Namespace Reservation Support (Base 2.4, 7.5, Figure 659, page 567, PDF page 597); bit 4 of Command Dword 10 */
#define XNVME_SPEC_IO_RESERVATION_ACQUIRE_CDW10_DISNSRS 0x10
/** Ignore Existing Key (Base 2.4, 7.5, Figure 659, page 567, PDF page 597); bit 3 of Command Dword 10 */
#define XNVME_SPEC_IO_RESERVATION_ACQUIRE_CDW10_IEKEY 0x8
/** Reservation Acquire Action (Base 2.4, 7.5, Figure 659, page 567, PDF page 597); bits 2:0 of Command Dword 10 */
#define XNVME_SPEC_IO_RESERVATION_ACQUIRE_CDW10_RACQA 0x7

/**
 * The values of Reservation Type (Base 2.4, 7.5, Figure 661, page 568, PDF page 598)
 */
enum xnvme_spec_io_reservation_acquire_cdw10_rtype {
	XNVME_SPEC_IO_RESERVATION_ACQUIRE_CDW10_RTYPE_WRITE_EXCLUSIVE_RESERVATION = 0x1, ///< Write Exclusive Reservation (Base 2.4, 7.5, Figure 661, page 568, PDF page 598)
	XNVME_SPEC_IO_RESERVATION_ACQUIRE_CDW10_RTYPE_EXCLUSIVE_ACCESS_RESERVATION = 0x2, ///< Exclusive Access Reservation (Base 2.4, 7.5, Figure 661, page 568, PDF page 598)
};

/**
 * The values of Reservation Acquire Action (Base 2.4, 7.5, Figure 659, page 567, PDF page 597)
 */
enum xnvme_spec_io_reservation_acquire_cdw10_racqa {
	XNVME_SPEC_IO_RESERVATION_ACQUIRE_CDW10_RACQA_ACQUIRE = 0x0, ///< Acquire (Base 2.4, 7.5, Figure 659, page 567, PDF page 597)
	XNVME_SPEC_IO_RESERVATION_ACQUIRE_CDW10_RACQA_PREEMPT = 0x1, ///< Preempt (Base 2.4, 7.5, Figure 659, page 567, PDF page 597)
	XNVME_SPEC_IO_RESERVATION_ACQUIRE_CDW10_RACQA_PREEMPT_AND_ABORT = 0x2, ///< Preempt and Abort (Base 2.4, 7.5, Figure 659, page 567, PDF page 597)
};

/*
 * The fields of I/O Management Receive, by the dwords of the command
 */
/** Management Operation Specific (Base 2.4, 7.3, Figure 651, page 564, PDF page 594); bits 31:16 of Command Dword 10 */
#define XNVME_SPEC_IO_IO_MANAGEMENT_RECEIVE_CDW10_MOS 0xffff0000
/** Management Operation (Base 2.4, 7.3, Figure 651, page 564, PDF page 594); bits 7:0 of Command Dword 10 */
#define XNVME_SPEC_IO_IO_MANAGEMENT_RECEIVE_CDW10_MO 0xff
/** Number of Dwords (Base 2.4, 7.3, Figure 652, page 564, PDF page 594); bits 31:0 of Command Dword 11 */
#define XNVME_SPEC_IO_IO_MANAGEMENT_RECEIVE_CDW11_NUMD 0xffffffff

/**
 * The values of Management Operation (Base 2.4, 7.3, Figure 651, page 564, PDF page 594)
 */
enum xnvme_spec_io_io_management_receive_cdw10_mo {
	XNVME_SPEC_IO_IO_MANAGEMENT_RECEIVE_CDW10_MO_RECLAIM_UNIT_HANDLE_STATUS = 0x1, ///< Reclaim Unit Handle Status (Base 2.4, 7.3, Figure 651, page 564, PDF page 594)
};

/*
 * The fields of Reservation Release, by the dwords of the command
 */
/** Reservation Type (Base 2.4, 7.7, Figure 666, page 570, PDF page 600); bits 15:8 of Command Dword 10 */
#define XNVME_SPEC_IO_RESERVATION_RELEASE_CDW10_RTYPE 0xff00
/** Dispersed Namespace Reservation Support (Base 2.4, 7.7, Figure 666, page 570, PDF page 600); bit 4 of Command Dword 10 */
#define XNVME_SPEC_IO_RESERVATION_RELEASE_CDW10_DISNSRS 0x10
/** Ignore Existing Key (Base 2.4, 7.7, Figure 666, page 570, PDF page 600); bit 3 of Command Dword 10 */
#define XNVME_SPEC_IO_RESERVATION_RELEASE_CDW10_IEKEY 0x8
/** Reservation Release Action (Base 2.4, 7.7, Figure 666, page 570, PDF page 600); bits 2:0 of Command Dword 10 */
#define XNVME_SPEC_IO_RESERVATION_RELEASE_CDW10_RRELA 0x7

/**
 * The values of Reservation Type (Base 2.4, 7.5, Figure 661, page 568, PDF page 598)
 */
enum xnvme_spec_io_reservation_release_cdw10_rtype {
	XNVME_SPEC_IO_RESERVATION_RELEASE_CDW10_RTYPE_WRITE_EXCLUSIVE_RESERVATION = 0x1, ///< Write Exclusive Reservation (Base 2.4, 7.5, Figure 661, page 568, PDF page 598)
	XNVME_SPEC_IO_RESERVATION_RELEASE_CDW10_RTYPE_EXCLUSIVE_ACCESS_RESERVATION = 0x2, ///< Exclusive Access Reservation (Base 2.4, 7.5, Figure 661, page 568, PDF page 598)
};

/**
 * The values of Reservation Release Action (Base 2.4, 7.7, Figure 666, page 570, PDF page 600)
 */
enum xnvme_spec_io_reservation_release_cdw10_rrela {
	XNVME_SPEC_IO_RESERVATION_RELEASE_CDW10_RRELA_RELEASE = 0x0, ///< Release (Base 2.4, 7.7, Figure 666, page 570, PDF page 600)
	XNVME_SPEC_IO_RESERVATION_RELEASE_CDW10_RRELA_CLEAR = 0x1, ///< Clear (Base 2.4, 7.7, Figure 666, page 570, PDF page 600)
};

/*
 * The fields of Cancel, by the dwords of the command
 */
/** Command Identifier (Base 2.4, 7.1, Figure 646, page 561, PDF page 591); bits 31:16 of Command Dword 10 */
#define XNVME_SPEC_IO_CANCEL_CDW10_CID 0xffff0000
/** Submission Queue Identifier (Base 2.4, 7.1, Figure 646, page 561, PDF page 591); bits 15:0 of Command Dword 10 */
#define XNVME_SPEC_IO_CANCEL_CDW10_SQID 0xffff
/** Action Code (Base 2.4, 7.1, Figure 647, page 562, PDF page 592); bits 1:0 of Command Dword 11 */
#define XNVME_SPEC_IO_CANCEL_CDW11_ACODE 0x3

/*
 * The fields of I/O Management Send, by the dwords of the command
 */
/** Management Operation Specific (Base 2.4, 7.4, Figure 655, page 565, PDF page 595); bits 31:16 of Command Dword 10 */
#define XNVME_SPEC_IO_IO_MANAGEMENT_SEND_CDW10_MOS 0xffff0000
/** Management Operation (Base 2.4, 7.4, Figure 655, page 565, PDF page 595); bits 7:0 of Command Dword 10 */
#define XNVME_SPEC_IO_IO_MANAGEMENT_SEND_CDW10_MO 0xff

/**
 * The values of Management Operation (Base 2.4, 7.4, Figure 655, page 565, PDF page 595)
 */
enum xnvme_spec_io_io_management_send_cdw10_mo {
	XNVME_SPEC_IO_IO_MANAGEMENT_SEND_CDW10_MO_RECLAIM_UNIT_HANDLE_UPDATE = 0x1, ///< Reclaim Unit Handle Update (Base 2.4, 7.4, Figure 655, page 565, PDF page 595)
};

/*
 * The fields of the feature Arbitration (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
 */
/** High Priority Weight (Base 2.4, 5.2.30.1.1, Figure 467, page 458, PDF page 486); bits 31:24 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_ARBITRATION_CDW11_HPW 0xff000000
/** Medium Priority Weight (Base 2.4, 5.2.30.1.1, Figure 467, page 458, PDF page 486); bits 23:16 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_ARBITRATION_CDW11_MPW 0xff0000
/** Low Priority Weight (Base 2.4, 5.2.30.1.1, Figure 467, page 458, PDF page 486); bits 15:8 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_ARBITRATION_CDW11_LPW 0xff00
/** Arbitration Burst (Base 2.4, 5.2.30.1.1, Figure 467, page 458, PDF page 486); bits 2:0 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_ARBITRATION_CDW11_AB 0x7

/*
 * The fields of the feature Power Management (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
 */
/** Idle I/O Exit Latency Limit (Base 2.4, 5.2.30.1.2, Figure 468, page 459, PDF page 487); bits 31:16 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_POWER_MANAGEMENT_CDW11_IIELL 0xffff0000
/** Workload Hint (Base 2.4, 5.2.30.1.2, Figure 468, page 459, PDF page 487); bits 7:5 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_POWER_MANAGEMENT_CDW11_WH 0xe0
/** Power State (Base 2.4, 5.2.30.1.2, Figure 468, page 459, PDF page 487); bits 4:0 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_POWER_MANAGEMENT_CDW11_PS 0x1f
/** Idle I/O Exit Latency Limit (Base 2.4, 5.2.30.1.2.1, Figure 469, page 460, PDF page 488); bits 31:16 of Completion Queue Entry Dword 0 */
#define XNVME_SPEC_BASE_FEATURE_POWER_MANAGEMENT_CQE_DW0_IIELL 0xffff0000
/** Workload Hint (Base 2.4, 5.2.30.1.2.1, Figure 469, page 460, PDF page 488); bits 7:5 of Completion Queue Entry Dword 0 */
#define XNVME_SPEC_BASE_FEATURE_POWER_MANAGEMENT_CQE_DW0_WH 0xe0
/** Power State (Base 2.4, 5.2.30.1.2.1, Figure 469, page 460, PDF page 488); bits 4:0 of Completion Queue Entry Dword 0 */
#define XNVME_SPEC_BASE_FEATURE_POWER_MANAGEMENT_CQE_DW0_PS 0x1f

/*
 * The fields of the feature LBA Range Type (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
 */
/** Number of LBA Ranges (NVM 1.3, 4.1.3.2, Figure 94, page 65, PDF page 65); bits 5:0 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_LBA_RANGE_TYPE_CDW11_NUM 0x3f
/** Number of LBA Ranges (NVM 1.3, 4.1.3.2, Figure 95, page 65, PDF page 65); bits 5:0 of Completion Queue Entry Dword 0 */
#define XNVME_SPEC_BASE_FEATURE_LBA_RANGE_TYPE_CQE_DW0_NUM 0x3f

/*
 * The fields of the feature Temperature Threshold (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
 */
/** Temperature Threshold Hysteresis (Base 2.4, 5.2.30.1.3.1, Figure 470, page 461, PDF page 489); bits 24:22 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_TEMPERATURE_THRESHOLD_CDW11_TMPTHH 0x1c00000
/** Threshold Type Select (Base 2.4, 5.2.30.1.3.1, Figure 470, page 461, PDF page 489); bits 21:20 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_TEMPERATURE_THRESHOLD_CDW11_THSEL 0x300000
/** Threshold Temperature Select (Base 2.4, 5.2.30.1.3.1, Figure 470, page 461, PDF page 489); bits 19:16 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_TEMPERATURE_THRESHOLD_CDW11_TMPSEL 0xf0000
/** Temperature Threshold (Base 2.4, 5.2.30.1.3.1, Figure 470, page 461, PDF page 489); bits 15:0 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_TEMPERATURE_THRESHOLD_CDW11_TMPTH 0xffff

/**
 * The values of Threshold Type Select (Base 2.4, 5.2.30.1.3.1, Figure 470, page 461, PDF page 489)
 */
enum xnvme_spec_base_feature_temperature_threshold_cdw11_thsel {
	XNVME_SPEC_BASE_FEATURE_TEMPERATURE_THRESHOLD_CDW11_THSEL_OVER_TEMPERATURE_THRESHOLD = 0x0, ///< Over Temperature Threshold (Base 2.4, 5.2.30.1.3.1, Figure 470, page 461, PDF page 489)
	XNVME_SPEC_BASE_FEATURE_TEMPERATURE_THRESHOLD_CDW11_THSEL_UNDER_TEMPERATURE_THRESHOLD = 0x1, ///< Under Temperature Threshold (Base 2.4, 5.2.30.1.3.1, Figure 470, page 461, PDF page 489)
};

/**
 * The values of Threshold Temperature Select (Base 2.4, 5.2.30.1.3.1, Figure 470, page 461, PDF page 489)
 */
enum xnvme_spec_base_feature_temperature_threshold_cdw11_tmpsel {
	XNVME_SPEC_BASE_FEATURE_TEMPERATURE_THRESHOLD_CDW11_TMPSEL_COMPOSITE_TEMPERATURE = 0x0, ///< Composite Temperature (Base 2.4, 5.2.30.1.3.1, Figure 470, page 461, PDF page 489)
	XNVME_SPEC_BASE_FEATURE_TEMPERATURE_THRESHOLD_CDW11_TMPSEL_TEMPERATURE_SENSOR_1 = 0x1, ///< Temperature Sensor 1 (Base 2.4, 5.2.30.1.3.1, Figure 470, page 461, PDF page 489)
	XNVME_SPEC_BASE_FEATURE_TEMPERATURE_THRESHOLD_CDW11_TMPSEL_TEMPERATURE_SENSOR_2 = 0x2, ///< Temperature Sensor 2 (Base 2.4, 5.2.30.1.3.1, Figure 470, page 461, PDF page 489)
	XNVME_SPEC_BASE_FEATURE_TEMPERATURE_THRESHOLD_CDW11_TMPSEL_TEMPERATURE_SENSOR_3 = 0x3, ///< Temperature Sensor 3 (Base 2.4, 5.2.30.1.3.1, Figure 470, page 461, PDF page 489)
	XNVME_SPEC_BASE_FEATURE_TEMPERATURE_THRESHOLD_CDW11_TMPSEL_TEMPERATURE_SENSOR_4 = 0x4, ///< Temperature Sensor 4 (Base 2.4, 5.2.30.1.3.1, Figure 470, page 461, PDF page 489)
	XNVME_SPEC_BASE_FEATURE_TEMPERATURE_THRESHOLD_CDW11_TMPSEL_TEMPERATURE_SENSOR_5 = 0x5, ///< Temperature Sensor 5 (Base 2.4, 5.2.30.1.3.1, Figure 470, page 461, PDF page 489)
	XNVME_SPEC_BASE_FEATURE_TEMPERATURE_THRESHOLD_CDW11_TMPSEL_TEMPERATURE_SENSOR_6 = 0x6, ///< Temperature Sensor 6 (Base 2.4, 5.2.30.1.3.1, Figure 470, page 461, PDF page 489)
	XNVME_SPEC_BASE_FEATURE_TEMPERATURE_THRESHOLD_CDW11_TMPSEL_TEMPERATURE_SENSOR_7 = 0x7, ///< Temperature Sensor 7 (Base 2.4, 5.2.30.1.3.1, Figure 470, page 461, PDF page 489)
	XNVME_SPEC_BASE_FEATURE_TEMPERATURE_THRESHOLD_CDW11_TMPSEL_TEMPERATURE_SENSOR_8 = 0x8, ///< Temperature Sensor 8 (Base 2.4, 5.2.30.1.3.1, Figure 470, page 461, PDF page 489)
};

/*
 * The fields of the feature Error Recovery (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
 */
/** Deallocated or Unwritten Logical Block Error Enable (NVM 1.3, 4.1.3.3, Figure 97, page 66, PDF page 66); bit 16 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_ERROR_RECOVERY_CDW11_DULBE 0x10000
/** Time Limited Error Recovery (NVM 1.3, 4.1.3.3, Figure 97, page 66, PDF page 66); bits 15:0 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_ERROR_RECOVERY_CDW11_TLER 0xffff

/*
 * The fields of the feature Volatile Write Cached (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
 */
/** Volatile Write Cache Enable (Base 2.4, 5.2.30.1.4, Figure 471, page 462, PDF page 490); bit 0 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_VOLATILE_WRITE_CACHED_CDW11_WCE 0x1

/*
 * The fields of the feature Number of Queues (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
 */
/** Number of I/O Completion Queues Requested (Base 2.4, 5.2.30.1.5, Figure 472, page 463, PDF page 491); bits 31:16 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_NUMBER_OF_QUEUES_CDW11_NCQR 0xffff0000
/** Number of I/O Submission Queues Requested (Base 2.4, 5.2.30.1.5, Figure 472, page 463, PDF page 491); bits 15:0 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_NUMBER_OF_QUEUES_CDW11_NSQR 0xffff
/** Number of I/O Completion Queues Allocated (Base 2.4, 5.2.30.1.5, Figure 473, page 464, PDF page 492); bits 31:16 of Completion Queue Entry Dword 0 */
#define XNVME_SPEC_BASE_FEATURE_NUMBER_OF_QUEUES_CQE_DW0_NCQA 0xffff0000
/** Number of I/O Submission Queues Allocated (Base 2.4, 5.2.30.1.5, Figure 473, page 464, PDF page 492); bits 15:0 of Completion Queue Entry Dword 0 */
#define XNVME_SPEC_BASE_FEATURE_NUMBER_OF_QUEUES_CQE_DW0_NSQA 0xffff

/*
 * The fields of the feature Interrupt Coalescing (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
 */
/** Aggregation Time (Base 2.4, 5.2.30.2.1, Figure 543, page 512, PDF page 541); bits 15:8 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_INTERRUPT_COALESCING_CDW11_TIME 0xff00
/** Aggregation Threshold (Base 2.4, 5.2.30.2.1, Figure 543, page 512, PDF page 541); bits 7:0 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_INTERRUPT_COALESCING_CDW11_THR 0xff

/*
 * The fields of the feature Interrupt Vector Configuration (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
 */
/** Coalescing Disable (Base 2.4, 5.2.30.2.2, Figure 544, page 512, PDF page 541); bit 16 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_INTERRUPT_VECTOR_CONFIGURATION_CDW11_CD 0x10000
/** Interrupt Vector (Base 2.4, 5.2.30.2.2, Figure 544, page 512, PDF page 541); bits 15:0 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_INTERRUPT_VECTOR_CONFIGURATION_CDW11_IV 0xffff

/*
 * The fields of the feature Write Atomicity Normal (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
 */
/** Disable Normal (NVM 1.3, 4.1.3.4, Figure 98, page 66, PDF page 66); bit 0 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_WRITE_ATOMICITY_NORMAL_CDW11_DN 0x1

/*
 * The fields of the feature Asynchronous Event Configuration (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
 */
/** Discovery Log Page Change Notification (Base 2.4, 5.2.30.1.6, Figure 474, page 464, PDF page 492); bit 31 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_ASYNCHRONOUS_EVENT_CONFIGURATION_CDW11_DLPCN 0x80000000
/** Host Discovery Log Page Change Notification (Base 2.4, 5.2.30.1.6, Figure 474, page 464, PDF page 492); bit 30 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_ASYNCHRONOUS_EVENT_CONFIGURATION_CDW11_HDLPCN 0x40000000
/** AVE Discovery Log Page Change Notification (Base 2.4, 5.2.30.1.6, Figure 474, page 464, PDF page 492); bit 29 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_ASYNCHRONOUS_EVENT_CONFIGURATION_CDW11_ADLPCN 0x20000000
/** Pull Model DDC Request Log Page Change Notification (Base 2.4, 5.2.30.1.6, Figure 474, page 464, PDF page 492); bit 28 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_ASYNCHRONOUS_EVENT_CONFIGURATION_CDW11_PMDRLPCN 0x10000000
/** Zone Descriptor Changed Notices2 (Base 2.4, 5.2.30.1.6, Figure 474, page 464, PDF page 492); bit 27 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_ASYNCHRONOUS_EVENT_CONFIGURATION_CDW11_ZDCN 0x8000000
/** Rate Limiting Configuration Change Notices1 (Base 2.4, 5.2.30.1.6, Figure 474, page 464, PDF page 492); bit 22 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_ASYNCHRONOUS_EVENT_CONFIGURATION_CDW11_RLCCN 0x400000
/** Allocated Namespace Attribute Notices (Base 2.4, 5.2.30.1.6, Figure 474, page 464, PDF page 492); bit 19 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_ASYNCHRONOUS_EVENT_CONFIGURATION_CDW11_ANSAN 0x80000
/** Reachability Group (Base 2.4, 5.2.30.1.6, Figure 474, page 464, PDF page 492); bit 18 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_ASYNCHRONOUS_EVENT_CONFIGURATION_CDW11_RGRP0 0x40000
/** Reachability Association (Base 2.4, 5.2.30.1.6, Figure 474, page 464, PDF page 492); bit 17 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_ASYNCHRONOUS_EVENT_CONFIGURATION_CDW11_RASSN 0x20000
/** Temperature Threshold Hysteresis Recovery (Base 2.4, 5.2.30.1.6, Figure 474, page 464, PDF page 492); bit 16 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_ASYNCHRONOUS_EVENT_CONFIGURATION_CDW11_TTHRY 0x10000
/** Normal NVM Subsystem Shutdown (Base 2.4, 5.2.30.1.6, Figure 474, page 464, PDF page 492); bit 15 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_ASYNCHRONOUS_EVENT_CONFIGURATION_CDW11_NNSSHDN 0x8000
/** Endurance Group Event Aggregate Log Change Notices (Base 2.4, 5.2.30.1.6, Figure 474, page 464, PDF page 492); bit 14 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_ASYNCHRONOUS_EVENT_CONFIGURATION_CDW11_EGEALCN 0x4000
/** LBA Status Information Alert Notices1 (Base 2.4, 5.2.30.1.6, Figure 474, page 464, PDF page 492); bit 13 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_ASYNCHRONOUS_EVENT_CONFIGURATION_CDW11_LSIAN 0x2000
/** Predictable Latency Event Aggregate Log Change Notices (Base 2.4, 5.2.30.1.6, Figure 474, page 464, PDF page 492); bit 12 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_ASYNCHRONOUS_EVENT_CONFIGURATION_CDW11_PLEALCN 0x1000
/** Asymmetric Namespace Access Change Notices (Base 2.4, 5.2.30.1.6, Figure 474, page 464, PDF page 492); bit 11 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_ASYNCHRONOUS_EVENT_CONFIGURATION_CDW11_ANACN 0x800
/** Telemetry Log Notices (Base 2.4, 5.2.30.1.6, Figure 474, page 464, PDF page 492); bit 10 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_ASYNCHRONOUS_EVENT_CONFIGURATION_CDW11_TLN 0x400
/** Firmware Activation Notices (Base 2.4, 5.2.30.1.6, Figure 474, page 464, PDF page 492); bit 9 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_ASYNCHRONOUS_EVENT_CONFIGURATION_CDW11_FAN 0x200
/** Attached Namespace Attribute Notices (Base 2.4, 5.2.30.1.6, Figure 474, page 464, PDF page 492); bit 8 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_ASYNCHRONOUS_EVENT_CONFIGURATION_CDW11_NAN 0x100
/** SMART / Health Critical Warnings (Base 2.4, 5.2.30.1.6, Figure 474, page 464, PDF page 492); bits 7:0 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_ASYNCHRONOUS_EVENT_CONFIGURATION_CDW11_SHCW 0xff

/*
 * The fields of the feature Autonomous Power State Transition (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
 */
/** Autonomous Power State Transition Enable (Base 2.4, 5.2.30.1.7, Figure 475, page 466, PDF page 494); bit 0 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_AUTONOMOUS_POWER_STATE_TRANSITION_CDW11_APSTE 0x1

/*
 * The fields of the feature Host Memory Buffer (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
 */
/** Cleared to Zero (Base 2.4, 5.2.30.2.3, Figure 545, page 513, PDF page 542); bit 3 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_HOST_MEMORY_BUFFER_CDW11_CTZ 0x8
/** Host Memory Non-operational Access Restriction Enable (Base 2.4, 5.2.30.2.3, Figure 545, page 513, PDF page 542); bit 2 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_HOST_MEMORY_BUFFER_CDW11_HMNARE 0x4
/** Memory Return (Base 2.4, 5.2.30.2.3, Figure 545, page 513, PDF page 542); bit 1 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_HOST_MEMORY_BUFFER_CDW11_MR 0x2
/** Enable Host Memory (Base 2.4, 5.2.30.2.3, Figure 545, page 513, PDF page 542); bit 0 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_HOST_MEMORY_BUFFER_CDW11_EHM 0x1
/** Host Memory Buffer Size (Base 2.4, 5.2.30.2.3, Figure 546, page 514, PDF page 543); bits 31:0 of Command Dword 12 */
#define XNVME_SPEC_BASE_FEATURE_HOST_MEMORY_BUFFER_CDW12_HSIZE 0xffffffff
/** Host Memory Descriptor List Lower Address (Base 2.4, 5.2.30.2.3, Figure 547, page 514, PDF page 543); bits 31:0 of Command Dword 13 */
#define XNVME_SPEC_BASE_FEATURE_HOST_MEMORY_BUFFER_CDW13_HMDLLA 0xffffffff
/** Host Memory Descriptor List Upper Address (Base 2.4, 5.2.30.2.3, Figure 548, page 514, PDF page 543); bits 31:0 of Command Dword 14 */
#define XNVME_SPEC_BASE_FEATURE_HOST_MEMORY_BUFFER_CDW14_HMDLUA 0xffffffff
/** Host Memory Descriptor List Entry Count (Base 2.4, 5.2.30.2.3, Figure 549, page 514, PDF page 544); bits 31:0 of Command Dword 15 */
#define XNVME_SPEC_BASE_FEATURE_HOST_MEMORY_BUFFER_CDW15_HMDLEC 0xffffffff
/** Host Memory Non-operational Access Restricted (Base 2.4, 5.2.30.2.3, Figure 552, page 515, PDF page 544); bit 3 of Completion Queue Entry Dword 0 */
#define XNVME_SPEC_BASE_FEATURE_HOST_MEMORY_BUFFER_CQE_DW0_HMNAR 0x8
/** Host Memory Non-operational Access Restriction Enable (Base 2.4, 5.2.30.2.3, Figure 552, page 515, PDF page 544); bit 2 of Completion Queue Entry Dword 0 */
#define XNVME_SPEC_BASE_FEATURE_HOST_MEMORY_BUFFER_CQE_DW0_HMNARE 0x4
/** Not Used (Base 2.4, 5.2.30.2.3, Figure 552, page 515, PDF page 544); bit 1 of Completion Queue Entry Dword 0 */
#define XNVME_SPEC_BASE_FEATURE_HOST_MEMORY_BUFFER_CQE_DW0_NUSED 0x2
/** Enable Host Memory (Base 2.4, 5.2.30.2.3, Figure 552, page 515, PDF page 544); bit 0 of Completion Queue Entry Dword 0 */
#define XNVME_SPEC_BASE_FEATURE_HOST_MEMORY_BUFFER_CQE_DW0_EHM 0x1

/*
 * The fields of the feature Keep Alive Timer (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
 */
/** Keep Alive Timeout (Base 2.4, 5.2.30.1.9, Figure 481, page 469, PDF page 497); bits 31:0 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_KEEP_ALIVE_TIMER_CDW11_KATO 0xffffffff

/*
 * The fields of the feature Host Controlled Thermal Management (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
 */
/** Thermal Management Temperature 1 (Base 2.4, 5.2.30.1.10, Figure 482, page 470, PDF page 498); bits 31:16 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_HOST_CONTROLLED_THERMAL_MANAGEMENT_CDW11_TMT1 0xffff0000
/** Thermal Management Temperature 2 (Base 2.4, 5.2.30.1.10, Figure 482, page 470, PDF page 498); bits 15:0 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_HOST_CONTROLLED_THERMAL_MANAGEMENT_CDW11_TMT2 0xffff

/*
 * The fields of the feature Non-Operational Power State Config (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
 */
/** Non-Operational Power State Permissive Mode Enable (Base 2.4, 5.2.30.1.11, Figure 483, page 470, PDF page 498); bit 0 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_NON_OPERATIONAL_POWER_STATE_CONFIG_CDW11_NOPPME 0x1

/*
 * The fields of the feature Read Recovery Level Config (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
 */
/** NVM Set Identifier (Base 2.4, 5.2.30.1.12, Figure 484, page 471, PDF page 499); bits 15:0 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_READ_RECOVERY_LEVEL_CONFIG_CDW11_NVMSETID 0xffff
/** Read Recovery Level (Base 2.4, 5.2.30.1.12, Figure 485, page 471, PDF page 499); bits 3:0 of Command Dword 12 */
#define XNVME_SPEC_BASE_FEATURE_READ_RECOVERY_LEVEL_CONFIG_CDW12_RRL 0xf

/*
 * The fields of the feature Predictable Latency Mode Config (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
 */
/** NVM Set Identifier (Base 2.4, 5.2.30.1.13, Figure 486, page 472, PDF page 500); bits 15:0 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_PREDICTABLE_LATENCY_MODE_CONFIG_CDW11_NVMSETID 0xffff
/** Predictable Latency Enable (Base 2.4, 5.2.30.1.13, Figure 487, page 472, PDF page 500); bit 0 of Command Dword 12 */
#define XNVME_SPEC_BASE_FEATURE_PREDICTABLE_LATENCY_MODE_CONFIG_CDW12_LPE 0x1

/*
 * The fields of the feature Predictable Latency Mode Window (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
 */
/** NVM Set Identifier (Base 2.4, 5.2.30.1.14, Figure 489, page 473, PDF page 501); bits 15:0 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_PREDICTABLE_LATENCY_MODE_WINDOW_CDW11_NVMSETID 0xffff
/** Window Select (Base 2.4, 5.2.30.1.14, Figure 490, page 473, PDF page 501); bits 2:0 of Command Dword 12 */
#define XNVME_SPEC_BASE_FEATURE_PREDICTABLE_LATENCY_MODE_WINDOW_CDW12_WSEL 0x7

/**
 * The values of Window Select (Base 2.4, 5.2.30.1.14, Figure 490, page 473, PDF page 501)
 */
enum xnvme_spec_base_feature_predictable_latency_mode_window_cdw12_wsel {
	XNVME_SPEC_BASE_FEATURE_PREDICTABLE_LATENCY_MODE_WINDOW_CDW12_WSEL_DETERMINISTIC_WINDOW = 0x1, ///< Deterministic Window (Base 2.4, 5.2.30.1.14, Figure 490, page 473, PDF page 501)
	XNVME_SPEC_BASE_FEATURE_PREDICTABLE_LATENCY_MODE_WINDOW_CDW12_WSEL_NON_DETERMINISTIC_WINDOW = 0x2, ///< Non-Deterministic Window (Base 2.4, 5.2.30.1.14, Figure 490, page 473, PDF page 501)
};

/*
 * The fields of the feature Sanitize Config (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
 */
/** No-Deallocate Response Mode (Base 2.4, 5.2.30.1.16, Figure 492, page 475, PDF page 503); bit 0 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_SANITIZE_CONFIG_CDW11_NODRM 0x1

/*
 * The fields of the feature Endurance Group Event Configuration (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
 */
/** Endurance Group Critical Warnings (Base 2.4, 5.2.30.1.17, Figure 493, page 476, PDF page 504); bits 23:16 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_ENDURANCE_GROUP_EVENT_CONFIGURATION_CDW11_EGCW 0xff0000
/** Endurance Group Identifier (Base 2.4, 5.2.30.1.17, Figure 493, page 476, PDF page 504); bits 15:0 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_ENDURANCE_GROUP_EVENT_CONFIGURATION_CDW11_ENDGID 0xffff

/*
 * The fields of the feature I/O Command Set Profile (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
 */
/** I/O Command Set Combination Index (Base 2.4, 5.2.30.1.18, Figure 494, page 477, PDF page 505); bits 8:0 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_IO_COMMAND_SET_PROFILE_CDW11_IOCSCI 0x1ff
/** I/O Command Set Combination Index (Base 2.4, 5.2.30.1.18, Figure 495, page 477, PDF page 505); bits 8:0 of Completion Queue Entry Dword 0 */
#define XNVME_SPEC_BASE_FEATURE_IO_COMMAND_SET_PROFILE_CQE_DW0_IOCSCI 0x1ff

/*
 * The fields of the feature Spinup Control (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
 */
/** Spinup Control Enable (Base 2.4, 5.2.30.1.19, Figure 496, page 477, PDF page 505); bit 0 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_SPINUP_CONTROL_CDW11_SCE 0x1

/*
 * The fields of the feature Power Loss Signaling Config (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
 */
/** Power Loss Signaling Mode (Base 2.4, 5.2.30.1.20, Figure 498, page 478, PDF page 506); bits 1:0 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_POWER_LOSS_SIGNALING_CONFIG_CDW11_PLSM 0x3

/*
 * The fields of the feature Performance Characteristics (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
 */
/** Revert Vendor Specific Performance Attribute (NVM 1.3, 4.1.3.8, Figure 102, page 69, PDF page 69); bit 8 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_PERFORMANCE_CHARACTERISTICS_CDW11_RVSPA 0x100
/** Attribute Index (NVM 1.3, 4.1.3.8, Figure 102, page 69, PDF page 69); bits 7:0 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_PERFORMANCE_CHARACTERISTICS_CDW11_ATTRI 0xff

/**
 * The values of Attribute Index (NVM 1.3, 4.1.3.8, Figure 102, page 69, PDF page 69)
 */
enum xnvme_spec_base_feature_performance_characteristics_cdw11_attri {
	XNVME_SPEC_BASE_FEATURE_PERFORMANCE_CHARACTERISTICS_CDW11_ATTRI_STANDARD_PERFORMANCE_ATTRIBUTE = 0x0, ///< Standard Performance Attribute (NVM 1.3, 4.1.3.8, Figure 102, page 69, PDF page 69)
	XNVME_SPEC_BASE_FEATURE_PERFORMANCE_CHARACTERISTICS_CDW11_ATTRI_PERFORMANCE_ATTRIBUTE_IDENTIFIER_LIST = 0xc0, ///< Performance Attribute Identifier List (NVM 1.3, 4.1.3.8, Figure 102, page 69, PDF page 69)
};

/*
 * The fields of the feature Flexible Data Placement (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
 */
/** Endurance Group Identifier (Base 2.4, 5.2.30.1.21, Figure 499, page 478, PDF page 506); bits 15:0 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_FLEXIBLE_DATA_PLACEMENT_CDW11_ENDGID 0xffff
/** Flexible Data Placement Configuration Index (Base 2.4, 5.2.30.1.21, Figure 500, page 478, PDF page 506); bits 15:8 of Command Dword 12 */
#define XNVME_SPEC_BASE_FEATURE_FLEXIBLE_DATA_PLACEMENT_CDW12_FDPCIDX 0xff00
/** Flexible Data Placement Enable (Base 2.4, 5.2.30.1.21, Figure 500, page 478, PDF page 506); bit 0 of Command Dword 12 */
#define XNVME_SPEC_BASE_FEATURE_FLEXIBLE_DATA_PLACEMENT_CDW12_FDPE 0x1

/*
 * The fields of the feature Flexible Data Placement Events (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
 */
/** Number of FDP Event Types (Base 2.4, 5.2.30.1.22, Figure 502, page 479, PDF page 507); bits 23:16 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_FLEXIBLE_DATA_PLACEMENT_EVENTS_CDW11_NOET 0xff0000
/** Placement Handle (Base 2.4, 5.2.30.1.22, Figure 502, page 479, PDF page 507); bits 15:0 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_FLEXIBLE_DATA_PLACEMENT_EVENTS_CDW11_PHNDL 0xffff
/** Number of FDP Event Types (Base 2.4, 5.2.30.1.22, Figure 501, page 479, PDF page 507); bits 7:0 of Completion Queue Entry Dword 0 */
#define XNVME_SPEC_BASE_FEATURE_FLEXIBLE_DATA_PLACEMENT_EVENTS_CQE_DW0_NOET 0xff

/*
 * The fields of the feature Key Value Configuration (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
 */
/** Error on Delete of Non-Existent KV Key (Key Value 1.4, 4.1.3.1, Figure 37, page 21, PDF page 21); bit 0 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_KEY_VALUE_CONFIGURATION_CDW11_EDNEK 0x1

/*
 * The fields of the feature Configurable Device Personality (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
 */
/** Change Personality Settings (Base 2.4, 5.2.30.1.25, Figure 513, page 483, PDF page 511); bit 9 of Command Dword 13 */
#define XNVME_SPEC_BASE_FEATURE_CONFIGURABLE_DEVICE_PERSONALITY_CDW13_CHPS 0x200
/** Personality Freeze State (Base 2.4, 5.2.30.1.25, Figure 513, page 483, PDF page 511); bit 8 of Command Dword 13 */
#define XNVME_SPEC_BASE_FEATURE_CONFIGURABLE_DEVICE_PERSONALITY_CDW13_PERFS 0x100
/** Personality Identifier (Base 2.4, 5.2.30.1.25, Figure 513, page 483, PDF page 511); bits 7:0 of Command Dword 13 */
#define XNVME_SPEC_BASE_FEATURE_CONFIGURABLE_DEVICE_PERSONALITY_CDW13_PERID 0xff

/**
 * The values of Personality Identifier (Base 2.4, 5.2.30.1.25, Figure 512, page 483, PDF page 511)
 */
enum xnvme_spec_base_feature_configurable_device_personality_cdw13_perid {
	XNVME_SPEC_BASE_FEATURE_CONFIGURABLE_DEVICE_PERSONALITY_CDW13_PERID_MANUFACTURING_DEFAULT_PERSONALITY = 0x0, ///< Manufacturing Default Personality (Base 2.4, 5.2.30.1.25, Figure 512, page 483, PDF page 511)
	XNVME_SPEC_BASE_FEATURE_CONFIGURABLE_DEVICE_PERSONALITY_CDW13_PERID_SECURITY_PERSONALITY = 0x1, ///< Security Personality (Base 2.4, 5.2.30.1.25, Figure 512, page 483, PDF page 511)
	XNVME_SPEC_BASE_FEATURE_CONFIGURABLE_DEVICE_PERSONALITY_CDW13_PERID_LOCKDOWN_PERSISTENCE_PERSONALITY = 0x2, ///< Lockdown Persistence Personality (Base 2.4, 5.2.30.1.25, Figure 512, page 483, PDF page 511)
	XNVME_SPEC_BASE_FEATURE_CONFIGURABLE_DEVICE_PERSONALITY_CDW13_PERID_ALL_PERSONALITIES = 0xff, ///< All Personalities (Base 2.4, 5.2.30.1.25, Figure 512, page 483, PDF page 511)
};

/*
 * The fields of the feature Power Limit (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
 */
/** Power Limit Scale (Base 2.4, 5.2.30.1.26, Figure 520, page 493, PDF page 522); bits 17:16 of Command Dword 13 */
#define XNVME_SPEC_BASE_FEATURE_POWER_LIMIT_CDW13_PLS 0x30000
/** Power Limit Value (Base 2.4, 5.2.30.1.26, Figure 520, page 493, PDF page 522); bits 15:0 of Command Dword 13 */
#define XNVME_SPEC_BASE_FEATURE_POWER_LIMIT_CDW13_PLV 0xffff

/**
 * The values of Power Limit Scale (Base 2.4, 5.2.30.1.26, Figure 520, page 493, PDF page 522)
 */
enum xnvme_spec_base_feature_power_limit_cdw13_pls {
	XNVME_SPEC_BASE_FEATURE_POWER_LIMIT_CDW13_PLS_0_0001_W = 0x1, ///< 0.0001 W (Base 2.4, 5.2.30.1.26, Figure 520, page 493, PDF page 522)
	XNVME_SPEC_BASE_FEATURE_POWER_LIMIT_CDW13_PLS_0_01_W = 0x2, ///< 0.01 W (Base 2.4, 5.2.30.1.26, Figure 520, page 493, PDF page 522)
};

/*
 * The fields of the feature Power Threshold (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
 */
/** Enable Power Threshold (Base 2.4, 5.2.30.1.27, Figure 521, page 494, PDF page 523); bit 31 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_POWER_THRESHOLD_CDW11_EPT 0x80000000
/** Power Measurement Type Select (Base 2.4, 5.2.30.1.27, Figure 521, page 494, PDF page 523); bits 23:20 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_POWER_THRESHOLD_CDW11_PMTS 0xf00000
/** Power Threshold Scale (Base 2.4, 5.2.30.1.27, Figure 521, page 494, PDF page 523); bits 17:16 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_POWER_THRESHOLD_CDW11_PTS 0x30000
/** Power Threshold Value (Base 2.4, 5.2.30.1.27, Figure 521, page 494, PDF page 523); bits 15:0 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_POWER_THRESHOLD_CDW11_PTV 0xffff

/*
 * The fields of the feature Power Measurement (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
 */
/** Stop Measurement Time (Base 2.4, 5.2.30.1.28, Figure 522, page 494, PDF page 524); bits 31:16 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_POWER_MEASUREMENT_CDW11_SMT 0xffff0000
/** Power Measurement Type Select (Base 2.4, 5.2.30.1.28, Figure 522, page 494, PDF page 524); bits 7:4 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_POWER_MEASUREMENT_CDW11_PMTS 0xf0
/** Action (Base 2.4, 5.2.30.1.28, Figure 522, page 494, PDF page 524); bits 3:0 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_POWER_MEASUREMENT_CDW11_ACT 0xf
/** Action Specific (Base 2.4, 5.2.30.1.28, Figure 523, page 495, PDF page 525); bits 31:0 of Command Dword 12 */
#define XNVME_SPEC_BASE_FEATURE_POWER_MEASUREMENT_CDW12_ACTSP 0xffffffff
/** Stop Measurement Time Remaining (Base 2.4, 5.2.30.1.28, Figure 524, page 496, PDF page 525); bits 31:16 of Completion Queue Entry Dword 0 */
#define XNVME_SPEC_BASE_FEATURE_POWER_MEASUREMENT_CQE_DW0_SMTR 0xffff0000
/** Power Measurement Generation Number (Base 2.4, 5.2.30.1.28, Figure 524, page 496, PDF page 525); bits 15:8 of Completion Queue Entry Dword 0 */
#define XNVME_SPEC_BASE_FEATURE_POWER_MEASUREMENT_CQE_DW0_PMGN 0xff00
/** Power Measurement Enable (Base 2.4, 5.2.30.1.28, Figure 524, page 496, PDF page 525); bit 0 of Completion Queue Entry Dword 0 */
#define XNVME_SPEC_BASE_FEATURE_POWER_MEASUREMENT_CQE_DW0_PME 0x1

/**
 * The values of Action (Base 2.4, 5.2.30.1.28, Figure 522, page 494, PDF page 524)
 */
enum xnvme_spec_base_feature_power_measurement_cdw11_act {
	XNVME_SPEC_BASE_FEATURE_POWER_MEASUREMENT_CDW11_ACT_STOP_POWER_MEASUREMENTS = 0x0, ///< Stop Power Measurements (Base 2.4, 5.2.30.1.28, Figure 522, page 494, PDF page 524)
	XNVME_SPEC_BASE_FEATURE_POWER_MEASUREMENT_CDW11_ACT_START_POWER_MEASUREMENTS = 0x1, ///< Start Power Measurements (Base 2.4, 5.2.30.1.28, Figure 522, page 494, PDF page 524)
};

/*
 * The fields of the feature Voltage Threshold (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
 */
/** Voltage Sensor Select (Base 2.4, 5.2.30.1.29, Figure 525, page 496, PDF page 525); bits 30:29 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_VOLTAGE_THRESHOLD_CDW11_VSENS 0x60000000
/** Enable Voltage Threshold (Base 2.4, 5.2.30.1.29, Figure 525, page 496, PDF page 525); bit 28 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_VOLTAGE_THRESHOLD_CDW11_EVT 0x10000000
/** Overvoltage Threshold (Base 2.4, 5.2.30.1.29, Figure 525, page 496, PDF page 525); bits 27:14 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_VOLTAGE_THRESHOLD_CDW11_OVT 0xfffc000
/** Undervoltage Threshold (Base 2.4, 5.2.30.1.29, Figure 525, page 496, PDF page 525); bits 13:0 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_VOLTAGE_THRESHOLD_CDW11_UVT 0x3fff

/*
 * The fields of the feature Voltage Measurement (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
 */
/** Action (Base 2.4, 5.2.30.1.30, Figure 526, page 498, PDF page 527); bits 3:0 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_VOLTAGE_MEASUREMENT_CDW11_ACT 0xf

/**
 * The values of Action (Base 2.4, 5.2.30.1.30, Figure 526, page 498, PDF page 527)
 */
enum xnvme_spec_base_feature_voltage_measurement_cdw11_act {
	XNVME_SPEC_BASE_FEATURE_VOLTAGE_MEASUREMENT_CDW11_ACT_STOP_VOLTAGE_MEASUREMENTS = 0x0, ///< Stop Voltage Measurements (Base 2.4, 5.2.30.1.30, Figure 526, page 498, PDF page 527)
	XNVME_SPEC_BASE_FEATURE_VOLTAGE_MEASUREMENT_CDW11_ACT_START_VOLTAGE_MEASUREMENTS = 0x1, ///< Start Voltage Measurements (Base 2.4, 5.2.30.1.30, Figure 526, page 498, PDF page 527)
	XNVME_SPEC_BASE_FEATURE_VOLTAGE_MEASUREMENT_CDW11_ACT_CLEAR_OVERVOLTAGE_VALID_AND_UNDERVOLTAGE_VALID = 0x2, ///< Clear Overvoltage Valid and Undervoltage Valid (Base 2.4, 5.2.30.1.30, Figure 526, page 498, PDF page 527)
};

/*
 * The fields of the feature Software Progress Marker (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
 */
/** Pre-boot Software Load Count (Base 2.4, 5.2.30.1.34, Figure 536, page 505, PDF page 534); bits 7:0 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_SOFTWARE_PROGRESS_MARKER_CDW11_PBSLC 0xff

/*
 * The fields of the feature Host Identifier (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
 */
/** Enable Extended Host Identifier (Base 2.4, 5.2.30.1.35, Figure 537, page 506, PDF page 535); bit 0 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_HOST_IDENTIFIER_CDW11_EXHID 0x1

/*
 * The fields of the feature Reservation Persistence (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
 */
/** Persist Through Power Loss (Base 2.4, 5.2.30.1.37, Figure 540, page 509, PDF page 538); bit 0 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_RESERVATION_PERSISTENCE_CDW11_PTPL 0x1

/*
 * The fields of the feature Boot Partition Write Protection Config (Base 2.4, 3.1.3.6, Figure 32, page 50, PDF page 76)
 */
/** Boot Partition 1 Write Protection State (Base 2.4, 5.2.30.1.39, Figure 542, page 510, PDF page 539); bits 5:3 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_BOOT_PARTITION_WRITE_PROTECTION_CONFIG_CDW11_BP1WPS 0x38
/** Boot Partition 0 Write Protection State (Base 2.4, 5.2.30.1.39, Figure 542, page 510, PDF page 539); bits 2:0 of Command Dword 11 */
#define XNVME_SPEC_BASE_FEATURE_BOOT_PARTITION_WRITE_PROTECTION_CONFIG_CDW11_BP0WPS 0x7

/**
 * The values of Boot Partition 1 Write Protection State (Base 2.4, 5.2.30.1.39, Figure 542, page 510, PDF page 539)
 */
enum xnvme_spec_base_feature_boot_partition_write_protection_config_cdw11_bp1wps {
	XNVME_SPEC_BASE_FEATURE_BOOT_PARTITION_WRITE_PROTECTION_CONFIG_CDW11_BP1WPS_WRITE_UNLOCKED = 0x1, ///< Write Unlocked (Base 2.4, 5.2.30.1.39, Figure 542, page 510, PDF page 539)
	XNVME_SPEC_BASE_FEATURE_BOOT_PARTITION_WRITE_PROTECTION_CONFIG_CDW11_BP1WPS_WRITE_LOCKED = 0x2, ///< Write Locked (Base 2.4, 5.2.30.1.39, Figure 542, page 510, PDF page 539)
};

/**
 * The values of Boot Partition 0 Write Protection State (Base 2.4, 5.2.30.1.39, Figure 542, page 510, PDF page 539)
 */
enum xnvme_spec_base_feature_boot_partition_write_protection_config_cdw11_bp0wps {
	XNVME_SPEC_BASE_FEATURE_BOOT_PARTITION_WRITE_PROTECTION_CONFIG_CDW11_BP0WPS_WRITE_UNLOCKED = 0x1, ///< Write Unlocked (Base 2.4, 5.2.30.1.39, Figure 542, page 510, PDF page 539)
	XNVME_SPEC_BASE_FEATURE_BOOT_PARTITION_WRITE_PROTECTION_CONFIG_CDW11_BP0WPS_WRITE_LOCKED = 0x2, ///< Write Locked (Base 2.4, 5.2.30.1.39, Figure 542, page 510, PDF page 539)
};

#endif /* LIBXNVME_SPEC_BASE_H */
