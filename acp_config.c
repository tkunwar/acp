/**
 * @file acp_config.c
 * @brief This file contains code which deals with parsing configuration file
 * and saves this information in acp_config structure which is globally
 * accessible.
 * @author Tej
 */
#include "acp_config.h"

/**
 * Initialise acp_config structure with default values
 */
static void init_acp_config() {
	acp_config.cell_compaction_enabled = true;
	acp_config.max_buff_size = 100;
	acp_config.swappiness = 70;
}
/**
 * Display the configuration that has been parsed from the configuration file.
 */
static void print_loaded_configs(){
	fprintf(stderr,"\nLoaded configs:");
	fprintf(stderr,"\n\tmax_buffer_size: %d",acp_config.max_buff_size);
	fprintf(stderr,"\n\tswappiness: %d",acp_config.swappiness);
	if (acp_config.cell_compaction_enabled == true){
		fprintf(stderr,"\n\tcell_compaction_enabled: true");
	}else{
		fprintf(stderr,"\n\tcell_compaction_enabled: false");
	}
	fprintf(stderr,"\n");
}
/**
 * Main routine for reading configuration file and save the information in
 * acp_config declared in acp_config.h .
 * @return Return ACP_OK if everything went OK else an error code.
 */
int load_config() {
	config_t cfg;
	const char* s_val; // will be used to store string values
	int val;
	bool config_fallback_enabled = false;
	// initialize acp_config state
	init_acp_config();

	/*Initialization */
	config_init(&cfg);
	/* Read the file. If there is an error, report it and exit. */
	if (!config_read_file(&cfg, ACP_CONF_FILE)) {
		printf("\n%s:%d - %s", config_error_file(&cfg), config_error_line(&cfg),
				config_error_text(&cfg));
		config_destroy(&cfg);
		return ACP_ERR_NO_CONFIG_FILE;
	}
	//read config_fallback_enabled field
	if (config_lookup_string(&cfg, "config_fallback_enabled", &s_val)) {
		if (strncmp(s_val, "true", 4) == 0) {
			config_fallback_enabled = true;
		}
	}else{
		fprintf(stderr,"\nWarning: Field conf_fallback_enabled not found in config file");
	}
	//read max_buffer_size
	if (config_lookup_int(&cfg, "max_buffer_size", &val)) {
		// if loaded value is acceptable to us, then save it
		if (val < 100) {
			fprintf(stderr, "\nError: max_buffer_size is too small");
			if (config_fallback_enabled == false) {
				// this is unacceptable, abort
				fprintf(stderr,
						"\nConfiguration prohibits using default values. Exiting!");
				return ACP_ERR_CONFIG_ABORT;
			}
		} else {
			acp_config.max_buff_size = val;
		}
	}else{
		fprintf(stderr,"\nWarning: Field max_buffer_size not found in config file!");
	}
	//read swappiness field
	if (config_lookup_int(&cfg, "swappiness", &val)) {
		if (val >= 80 && val <= 95) {
			fprintf(stderr,
					"\nWarning: swappiness value is too high.\nPerformance may be affected!");
			acp_config.swappiness = val;
		} else if (val > 95) {
			fprintf(stderr, "\nError: swappiness value is too high.");
			if (config_fallback_enabled == false) {
				fprintf(stderr,
						"\nConfiguration prohibits using default values. Exiting!");
				return ACP_ERR_CONFIG_ABORT;
			}
		}
		else{
			acp_config.swappiness = val;
		}
	}else{
		fprintf(stderr,"\nWarning: Field swappiness not found in config file!");
	}
	//read cell_compaction_enabled
		if (config_lookup_string(&cfg, "cell_compaction_enabled", &s_val)) {
			if (strncmp(s_val, "true", 4) == 0) {
				acp_config.cell_compaction_enabled = true;
			}
			else{
				acp_config.cell_compaction_enabled = false;
			}
		}
		else{
			fprintf(stderr,"\nWarning: Field cell_compaction_enabled not found in config file!");
		}
	config_destroy(&cfg);
	print_loaded_configs();
	return ACP_OK;
}
