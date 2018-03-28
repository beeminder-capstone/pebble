Pebble.addEventListener('showConfiguration', function () {
	var url = 'https://www.beeminder.com/apps/authorize?client_id=b4t29518iw7a2weh97wt5wcbj&redirect_uri=https://www.beeminder.com/pebble/beeminder.html&response_type=token';

	Pebble.openURL(url);
});

Pebble.addEventListener('webviewclosed', function (e) {
	// Decode the user's preferences
	var configData = JSON.parse(decodeURIComponent(e.response));

	// Send to the watchapp via AppMessage
	var dict = {
		'access_token': configData.access_token,
		'username': configData.username,
		'goal': configData.goal,
		'metric': configData.metric
	};

	// Send to the watchapp
	Pebble.sendAppMessage(dict, function () {
		console.log('Config data sent successfully!');
	}, function (e) {
		console.log('Error sending config data!');
	});
});

var xhrRequest = function (url, type, callback) {
	var xhr = new XMLHttpRequest();
	xhr.onload = function () {
		callback(this.responseText);
	};
	xhr.open(type, url);
	xhr.send();
};

function getgoal(access_token, goal) {
	// Construct URL
	var url = "https://www.beeminder.com/api/v1/users/me/goals/" + goal + ".json?access_token=" + access_token;

	// Send request to Beeminder
	xhrRequest(url, 'GET',
		function (responseText) {
			// responseText contains a JSON object with goal info
			var json = JSON.parse(responseText);
			//console.log(responseText);

			var rate = json.rate;

			var roadstatuscolor = json.roadstatuscolor;

			var limsum = json.limsum;

			// Assemble dictionary using our keys
			var dictionary = {
				"rate": rate,
				"roadstatuscolor": roadstatuscolor,
				"limsum": limsum
			};

			// Send to Pebble
			Pebble.sendAppMessage(dictionary,
				function (e) {
					console.log("Goal info sent to Pebble successfully!");
				},
				function (e) {
					console.log("Error sending goal info to Pebble!");
				}
			);
		}
	);
}

function postgoal(access_token, goal, timestamp, value, datetime) {
	// Construct URL
	var url = "https://www.beeminder.com/api/v1/users/me/goals/" + goal + "/datapoints.json?access_token=" + access_token + "&timestamp=" + timestamp + "&value=" + value;

	// Send request to Beeminder
	xhrRequest(url, 'POST',
		function (responseText) {
			// responseText contains a JSON object with goal info
			var json = JSON.parse(responseText);
			//console.log(responseText);

			var errors = json.errors;

			var title = 'Beeminder';
			var body;

			if (errors)
				body = 'An error occured adding a datapoint to goal ' + goal + ': ' + errors.message + '.';
			else
				body = 'A datapoint was added to goal ' + goal + ' with value ' + value + ' for ' + datetime + '.';

			// Show the notification
			Pebble.showSimpleNotificationOnPebble(title, body);
		}
	);
}


// Listen for when the watchface is opened
Pebble.addEventListener('ready',
	function (e) {
		console.log('PebbleKit JS ready!');

		// Update s_js_ready on watch
		Pebble.sendAppMessage({ 'JSReady': 1 });
	}
);

// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
	function (e) {
		// Get the dictionary from the message
		var dict = e.payload;

		var access_token;
		var goal;
		var timestamp;
		var value;
		var datetime;

		if (dict[0] && dict[1]) {
			// The RequestData key is present, read the value
			access_token = dict[0];
			goal = dict[1];

			getgoal(access_token, goal);
		}

		if (dict[2] && dict[3] && dict[4] && dict[5] && dict[6]) {
			// The RequestData key is present, read the value
			access_token = dict[2];
			goal = dict[3];
			timestamp = dict[4];
			value = dict[5];
			datetime = dict[6];

			postgoal(access_token, goal, timestamp, value, datetime);
		}

		console.log('AppMessage received!');
	}
);
