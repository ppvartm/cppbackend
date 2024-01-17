#include "postgres.h"



namespace postgres_tools {

	void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, const Record& record) {
		jv = {
		{"name", record.name},
		{"score", record.score},
		{"playTime", record.play_time}
		};
	}

}