
/*Hack the ebml head*/
struct 	ebml_head {
	/*level 0*/
	char ebml_head_id[4];
	char ebml_head_size[1];
	/*level 1*/
	char ebml_doctype_id[2];
	char ebml_doctype_size[1];
	char ebml_doctype_data[8];
	/*level 1*/
	char ebml_doctype_v_id[2];
	char ebml_doctype_v_size[1];
	char ebml_doctype_v_data[1];
	/*level 1*/
        char ebml_doctype_rv_id[2];
        char ebml_doctype_rv_size[1];
        char ebml_doctype_rv_data[1];
};/*end ebml head*/

struct ebml_void {
	char ebml_void_id[1];
	char ebml_void_size[2];
	/*fill with 0 according to the size*/
};

struct seek_head {
	/*level 1*/
	char seek_head_id[4];
	char seek_head_size[4];
};

struct seek_entry {
	char seek_entry_id[2];
	char seek_entry_size[1];

	char seek_id[2];
	char seek_size[1];
	char seek_data[4];

	char seek_position_id[2];
	char seek_position_size[1];
	char seek_position_data[4];
};

/*Hack the segment*/
struct segment {
	/*level 0*/
	char segment_id[4];
	char segment_size[8];
};

/*Hack the segment info*/
struct segment_info {
	/*level 1*/
	char info_id[4];
	char info_size[2];
	/*level 2*/
	char info_timescale_id[3];
	char info_timescale_size[1];
	char info_timescale_data[3];
	/*level 2*/
	char uid_id[2];
	char uid_size[1];
	char uid_data[16];
	/*level 2*/
	char info_muxingapp_id[2];
        /*our mux app is "make_mkv based on libebml + libmatroska by weijian@mail.hhcn.com"*/
	/*so the size is 0xC0(vint) 0x40*/
	char info_muxingapp_size[1];
	char info_muxingapp_data[64];
	/*level 2*/
	char info_writingapp_id[2];
	/*our write app is "make_mkv build by weijian@mail.hhcn.com"*/
	/*so the size is 0xA7(vint) 0x27*/
	char info_writingapp_size[1];
	char info_writingapp_data[39];
};/*end segment info*/

struct cue {
	/*level 1*/
	char cue_id[4];
	char cue_size[4];
};

struct cue_point {
	/*level 2*/
	char cue_point_id[1];
	char cue_point_size[1];

	/*level 3*/
	char cue_time_id[1];
	char cue_time_size[1];
	char cue_time_data[4];

	/*level 3*/
	char cue_track_pos_id[1];
	char cue_track_pos_size[1];

	/*level 4*/
	char cue_track_num_id[1];
	char cue_track_num_size[1];
	char cue_track_num_data[1];

	/*level 4*/
	char cue_cluster_pos_id[1];
	char cue_cluster_pos_size[1];
	char cue_cluster_pos_data[4];
};

struct track {
	/*level 1*/
	char track_id[4];
	char track_size[2];
};

/*Hack the video track*/
struct video_track {
	/*level 2*/
	char track_entry_id[1];
	char track_entry_size[2];

	/*level 3*/
	char track_number_id[1];
	char track_number_size[1];
	char track_number_data[1];

	/*level 3*/
	char track_uid_id[2];
	char track_uid_size[1];
	char track_uid_data[4];

	/*level 3*/
	char track_type_id[1];
	char track_type_size[1];
	char track_type_data[1];

	/*level 3*/
	char track_enable_id[1];
	char track_enable_size[1];
	char track_enable_data[1];

        /*level 3*/
        char track_default_id[1];
        char track_default_size[1];
        char track_default_data[1];

        /*level 3*/
        char track_forced_id[2];
        char track_forced_size[1];
        char track_forced_data[1];

        /*level 3*/
        char track_lacing_id[1];
        char track_lacing_size[1];
        char track_lacing_data[1];

        /*level 3*/
        char track_mincache_id[2];
        char track_mincache_size[1];
        char track_mincache_data[1];

        /*level 3*/
        char track_timescale_id[3];
        char track_timescale_size[1];
        char track_timescale_data[4];

	/*level 3*/
	char track_codecid_id[1];
	char track_codecid_size[1];
	/*V_MPEG4/ISO/AVC*/
	char track_codecid_data[15];

	/*level 3*/
	/*we hack the 264 codecprivate data*/
	char track_codecprivate_id[2];
	char track_codecprivate_size[1];
	char track_codecpirvate_data[24];

	/*level 3*/
	char track_duration_id[3];
	char track_duration_size[1];
	char track_duration_data[4];

	/*level 3*/
	char track_lang_id[3];
	char track_lang_size[1];
	char track_lang_data[3];

#if 0
	/*level 3*/
	char track_video_id[1];
	char track_video_size[1];

	/*level 4*/
	char pixel_width_id[1];
	char pixel_width_size[1];
	char pixel_width_data[2];
        /*level 4*/
        char pixel_height_id[1];
        char pixel_height_size[1];
        char pixel_height_data[2];
	/*level 4*/
	char interlaced_id[1];
	char interlaced_size[1];
	char interlaced_data[1];
	/*level 4*/
        char display_width_id[2];
        char display_width_size[1];
        char display_width_data[4];
        /*level 4*/
        char display_height_id[2];
        char display_height_size[1];
        char display_height_data[4];
#endif
};

/*Hack the audio track*/
struct audio_track {
	/*level 2*/
	char track_entry_id[1];
	char track_entry_size[2];

	/*level 3*/
	char track_number_id[1];
	char track_number_size[1];
	char track_number_data[1];

	/*level 3*/
	char track_uid_id[2];
	char track_uid_size[1];
	char track_uid_data[4];

	/*level 3*/
	char track_type_id[1];
	char track_type_size[1];
	char track_type_data[1];

	/*level 3*/
	char track_enable_id[1];
	char track_enable_size[1];
	char track_enable_data[1];

        /*level 3*/
        char track_default_id[1];
        char track_default_size[1];
        char track_default_data[1];

        /*level 3*/
        char track_forced_id[2];
        char track_forced_size[1];
        char track_forced_data[1];

        /*level 3*/
        char track_lacing_id[1];
        char track_lacing_size[1];
        char track_lacing_data[1];

        /*level 3*/
        char track_mincache_id[2];
        char track_mincache_size[1];
        char track_mincache_data[1];

        /*level 3*/
        char track_timescale_id[3];
        char track_timescale_size[1];
        char track_timescale_data[4];

	/*level 3*/
	char track_codecid_id[1];
	char track_codecid_size[1];
	char track_codecid_data[8];

	/*level 3*/
	char track_lang_id[3];
	char track_lang_size[1];
	char track_lang_data[3];

	/*level 3*/
	char track_audio_id[1];
	char track_audio_size[1];

	/*level 4*/
	char samp_freq_id[1];
	char samp_freq_size[1];
	char samp_freq_data[4];
	/*levle 4*/
	char channels_id[1];
	char channels_size[1];
	char channels_data[1];
	/*levle 4*/
	char out_freq_id[2];
	char out_freq_size[1];
	char out_freq_data[4];
};

/*Hack the srt track*/
struct subtitle_track {
	/*level 2*/
	char track_entry_id[1];
	char track_entry_size[2];

	/*level 3*/
	char track_number_id[1];
	char track_number_size[1];
	char track_number_data[1];

	/*level 3*/
	char track_uid_id[2];
	char track_uid_size[1];
	char track_uid_data[4];

	/*level 3*/
	char track_type_id[1];
	char track_type_size[1];
	char track_type_data[1];

	/*level 3*/
	char track_enable_id[1];
	char track_enable_size[1];
	char track_enable_data[1];

        /*level 3*/
        char track_default_id[1];
        char track_default_size[1];
        char track_default_data[1];

        /*level 3*/
        char track_forced_id[2];
        char track_forced_size[1];
        char track_forced_data[1];

        /*level 3*/
        char track_lacing_id[1];
        char track_lacing_size[1];
        char track_lacing_data[1];

        /*level 3*/
        char track_mincache_id[2];
        char track_mincache_size[1];
        char track_mincache_data[1];

        /*level 3*/
        char track_timescale_id[3];
        char track_timescale_size[1];
        char track_timescale_data[4];

	/*level 3*/
	char track_codecid_id[1];
	char track_codecid_size[1];
	char track_codecid_data[11];

	/*level 3*/
	char track_lang_id[3];
	char track_lang_size[1];
	char track_lang_data[3];
};

struct cluster{
	/*level 1(cluster)*/
	char cluster_id[4];
	/*we set it unkown*/
	char cluster_size[4];

	/*level 2(TimeCode)*/
	char timecode_id[1];
	char timecode_size[1];
	char timecode_data[4];
};

struct simple_block{
	/*level 2*/
	char simple_block_id[1];
	/*vint*/
	char simple_block_size[4];

	/*Block header*/
	char track_num[1];
	char relative_timecode[2];
	char flag[1];
	char encode_data_size[4];
	/*encode_data*/
};

/*block_group for subtitle*/
struct block_group{
	/*level 2*/
	char block_group_id[1];
	/*vint*/
	char block_group_size[4];

	/*level 3*/
	char block_dur_id[1];
	/*vint*/
	char block_dur_size[1];
	char block_dur_data[2];

	/*level 3*/
	char block_id[1];
	/*vint */
	char block_size[1];

	/*Group header*/
	char track_num[1];
	char relative_timecode[2];
	char flag[1];
	/*srt string*/
	char time_string[8];
};

