// App Info
var version = "3.5";

// Default Server
var server = '';

// Global Settings Variables
var favoriteTeam = 19;
var shakeEnabled = 1;
var shakeTime = 5;
var refreshTime = [3600, 60];
var basesDisplay = 1;
var primaryColor = "FFFFFF";
var secondaryColor = "FFFFFF";
var backgroundColor = "AA0000";
/*
* Future Settings:
*/
// var vibrateOnChange
// var vibrateOnLeadChange
// var vibrateOnGameEnd
// var vibrateOnGameStart

// Global Variables
var offset = 0;
var teams = ["", "LAA", "HOU", "OAK", "TOR", "ATL", "MIL", "STL", "CHC", "ARI", "LAD", "SF", "CLE", "SEA", "MIA", "NYM", "WSH", "BAL", "SD", "PHI", "PIT", "TEX", "TB", "BOS", "CIN", "COL", "KC", "DET", "MIN", "CWS", "NYY"];
var retry = 0;

// Function to get the timezone offset
function getTimezoneOffsetHours(){
  var offsetHours = new Date().getTimezoneOffset() / 60;
  offsetHours = offsetHours - 4;
  return offsetHours;
}

// Function to parse a score
function parseScore(raw_score){
  var score = parseInt(raw_score);
  if(score === ""){
    score = 0;
  }
  return score;
}

// Function to send a message to the Pebble using AppMessage API
function sendDataToWatch(data){
	Pebble.sendAppMessage(data);
}

// Function to build the dictionary of relavant data for the AppMessage
function compileDataForWatch(raw_data, game){
  // Determine how much the data has changed
  // Prepare the dictionary depending on game status
  // Compile dictionary based on what has changed
  retry = 0;
  var data = raw_data.game_data[game];
  var game_status = data.game_status;
  var dictionary = {'TYPE':1};
  
  if (game_status == 'Preview' || game_status == 'Warmup' || game_status == 'Pre-Game' || game_status == 'Postponed' || game_status == 'Cancelled'){
    
    dictionary.NUM_GAMES = 1;
    if (game_status == 'Preview'){
      dictionary.STATUS = 0;
    } else {
      dictionary.STATUS = 1; 
    }
    dictionary.HOME_TEAM = data.home_team;
    dictionary.AWAY_TEAM = data.away_team;
    dictionary.HOME_PITCHER = data.home_pitcher;
    dictionary.AWAY_PITCHER = data.away_pitcher;
    if (game_status == 'Postponed'){
      dictionary.GAME_TIME = "PPD";
    } else if (game_status == 'Cancelled'){
      dictionary.GAME_TIME = "CAN";
    } else {
      dictionary.GAME_TIME = data.game_time;
    }
    if(dictionary.HOME_TEAM == teams[favoriteTeam]){
      dictionary.HOME_BROADCAST = data.home_radio_broadcast;
      dictionary.AWAY_BROADCAST = data.home_tv_broadcast;
    } else {
      dictionary.HOME_BROADCAST = data.away_radio_broadcast;
      dictionary.AWAY_BROADCAST = data.away_tv_broadcast;
    }
    sendDataToWatch(dictionary);
    
  } else if (game_status == 'In Progress'){
    
    dictionary.NUM_GAMES = 1;
    dictionary.STATUS = 2;
    dictionary.HOME_TEAM = data.home_team;
    dictionary.AWAY_TEAM = data.away_team;
    dictionary.FIRST = parseInt(data.runners_on_base.split(":")[0]);
    dictionary.SECOND = parseInt(data.runners_on_base.split(":")[1]);
    dictionary.THIRD = parseInt(data.runners_on_base.split(":")[2]);
    dictionary.HOME_SCORE = parseScore(data.home_score);
    dictionary.AWAY_SCORE = parseScore(data.away_score);
    dictionary.INNING = parseInt(data.inning);
    dictionary.INNING_HALF = data.inning_half;
    dictionary.BALLS = parseInt(data.balls);
    dictionary.STRIKES = parseInt(data.strikes);
    dictionary.OUTS = parseInt(data.outs);
    sendDataToWatch(dictionary);
    
  } else {
    
    dictionary.NUM_GAMES = 1;
    dictionary.STATUS = 3;
    dictionary.HOME_TEAM = data.home_team;
    dictionary.AWAY_TEAM = data.away_team;
    dictionary.HOME_SCORE = parseScore(data.home_score);
    dictionary.AWAY_SCORE = parseScore(data.away_score);
    dictionary.INNING = parseInt(data.inning);
    sendDataToWatch(dictionary);
    
  }
}

function within_30_minutes(game_time){
  var game_hours = game_time.split(":")[0];
  var game_minutes = game_time.split(":")[1];
  var now = new Date();
  var hours = now.getHours();
  if (hours > 12) {
    hours = hours - 12;
  }
  var minutes = now.getMinutes();
  if (game_hours > hours + 1){
    return false;
  } else if (game_hours > hours) {
    if ((60 + game_hours) - minutes > 30){
      return false;
    } else {
      return true;
    }
  } else if (game_hours == hours){
    if (game_minutes > minutes + 30){
      return false;
    } else {
      return true;
    }
  } else {
    return true;
  }
}

function chooseGame(data){
  // Initial variable set
  var game = 0;
  
  // Get game information
  var game_1_status = data.game_data[0].game_status;
  var game_2_status = data.game_data[1].game_status;
  var game_2_time = data.game_data[1].game_time;
  
  // Determine which game to use
  if (game_1_status == 'Preview' || game_1_status == 'Warmnp' || game_1_status == 'In Progress' || game_1_status == 'Pre-Game'){
    // If first game has not started yet or is in progress, use first game
    game = 0;
  } else if (game_1_status == 'Final' && within_30_minutes(game_2_time) === false){
    // If first game has ended and next game is more than 30 minutes away, show first game
    game = 0;
  } else if (game_1_status == 'Final' && within_30_minutes(game_2_time) === true){
    // If first game is over and next game is less than 30 minutes away or any other status, show second game
    game = 1;
  }  else if (game_2_status == 'In Progress'){
    // Game 2 in progress fallback
    game = 1;
  } else {
    // Fallback
    game = 1;
  }
  
  // Catch, if offset is -1 then show second game
  if (game_2_status == 'Final'){
    // Game 2 final fallback
    game = 1;
  }
  
  return game;
}

// Function to correct game time based on timezone
function correctTimezone(data){
  if(data.number_of_games > 0){
    var game_number = 0;
    if(data.number_of_games > 1){
      game_number = chooseGame(data);
    }
    var game_time = data.game_data[game_number].game_time;
    var game_hours = game_time.split(":")[0];
    var new_game_hours = parseInt(game_hours) - getTimezoneOffsetHours();
    if (new_game_hours > 12){
      new_game_hours = new_game_hours - 12;
    } else if (new_game_hours < 1){
      new_game_hours = new_game_hours + 12;
    }
    data.game_data[game_number].game_time = new_game_hours + ":" + game_time.split(":")[1];
    return data;
  } else {
    return data;
  }
}

// Function to process the incoming data
function processGameData(gameData){
  // Route incoming data by number of games
  var data = correctTimezone(gameData);
  var number_of_games = data.number_of_games;
  // Route the game data based on number of games
  if (number_of_games === 0) {
    // If no game, only report no game
    var dictionary = {
      'TYPE':1,
      'NUM_GAMES': 0
    };
    sendDataToWatch(dictionary);
  } else if (number_of_games == 1){
    // If one game, determine information to send to watch
    compileDataForWatch(data, 0);
  } else{
    // If multiple games, determine which game is relavent
    compileDataForWatch(data, chooseGame(data));
  }
}

// Function to get MLB data from personal server
function getGameData(offset){
  var method = 'GET';
  var watch = Pebble.getActiveWatchInfo ? Pebble.getActiveWatchInfo() : null;
  var url = 'http://pebble.' + server + '.chs.network/mlb/api-3.php?cst=' + Pebble.getAccountToken() + '&team=' + teams[favoriteTeam] + '&offset=' + offset + '&platform=' + watch.platform + '&version=' + version;
  // Create the request
  var request = new XMLHttpRequest();
  
  // Specify the callback for when the request is completed
  request.onload = function() {
    // The request was successfully completed!
    var raw_data = JSON.parse(this.responseText);
    var number_of_games = parseInt(JSON.parse(raw_data.number_of_games));
    if(offset == -1 && number_of_games === 0){
      offset = 0;
      getGameData(offset);
    } else if(number_of_games === 0 && retry === 0){
      getGameData(offset);
      retry = 1;
    } else if(offset == -1 && number_of_games == 1 && (raw_data.game_data[0].game_status == 'Postponed' || raw_data.game_data[0].game_status == 'Cancelled')){
      offset = 0;
      getGameData(offset);
    } else if(offset == -1 && number_of_games == 2 && (raw_data.game_data[1].game_status == 'Postponed' || raw_data.game_data[1].game_status == 'Cancelled')){
      offset = 0;
      getGameData(offset);
    } else {
      processGameData(raw_data);
    }
  };
  
  // Send the request
  request.open(method, url);
  request.send();
}

// Function called to refresh game data
function newGameDataRequest(){
  // Get Timezone offset and convert to eastern time
  var now = new Date();
  var hours = now.getHours() + getTimezoneOffsetHours();
  if(hours > 23){
    hours = hours - 24;
  }
  // Determine if before 11 am EST
  if (hours > 10) {
    // If after 10:59, offset 0
    offset = 0;
    getGameData(offset);
  } else {
    // If before 11:00, offset -1
    offset = -1;
    getGameData(offset);
  }
}

// Function to append settings to data
function sendSettings(){
  var dictionary = {'TYPE':0, 'PREF_FAVORITE_TEAM':parseInt(favoriteTeam), 'PREF_SHAKE_ENABELED':parseInt(shakeEnabled), 'PREF_SHAKE_TIME':parseInt(shakeTime), 'PREF_REFRESH_TIME_OFF':parseInt(refreshTime[0]), 'PREF_REFRESH_TIME_ON': parseInt(refreshTime[1]), 'PREF_PRIMARY_COLOR': primaryColor, 'PREF_SECONDARY_COLOR': secondaryColor, 'PREF_BACKGROUND_COLOR': backgroundColor, 'PREF_BASES_DISPLAY': parseInt(basesDisplay)};
  sendDataToWatch(dictionary);
}

// Function to load the stored settings
function loadSettings(){
  favoriteTeam = localStorage.getItem(1);
  if(favoriteTeam === null){
    favoriteTeam = 19;
  }
  shakeEnabled = localStorage.getItem(2);
  if(shakeEnabled === null){
    shakeEnabled = 1;
  }
  shakeTime = localStorage.getItem(3);
  if(shakeTime === null){
    shakeTime = 5;
  }
  refreshTime[0] = localStorage.getItem(4);
  if(refreshTime[0] === null){
    refreshTime[0] = 3600;
  }
  refreshTime[1] = localStorage.getItem(5);
  if(refreshTime[1] === null){
    refreshTime[1] = 60;
  }
  primaryColor = localStorage.getItem(6);
  if(primaryColor === null){
    primaryColor = "FFFFFF";
  }
  secondaryColor = localStorage.getItem(7);
  if(secondaryColor === null){
    secondaryColor = "FFFFFF";
  }
  backgroundColor = localStorage.getItem(8);
  if(backgroundColor === null){
    backgroundColor = "AA0000";
  }
  basesDisplay = localStorage.getItem(9);
  if(basesDisplay === null){
    basesDisplay = 1;
  }
  sendSettings();
}

// Function to store settings
function storeSettings(configuration){
  if (configuration.hasOwnProperty('favorite_team') === true) {
    favoriteTeam = parseInt(configuration.favorite_team);
    localStorage.setItem(1, favoriteTeam);
  }
  if (configuration.hasOwnProperty('shake_enabeled') === true) {
    shakeEnabled = parseInt(configuration.shake_enabeled);
    localStorage.setItem(2, shakeEnabled);
  }
  if (configuration.hasOwnProperty('shake_time') === true) {
    shakeTime = parseInt(configuration.shake_time);
    localStorage.setItem(3, shakeTime);
  }
  if (configuration.hasOwnProperty('refresh_off') === true) {
    refreshTime[0] = parseInt(configuration.refresh_off);
    localStorage.setItem(4, refreshTime[0]);
  }
  if (configuration.hasOwnProperty('refresh_game') === true) {
    refreshTime[1] = parseInt(configuration.refresh_game);
    localStorage.setItem(5, refreshTime[1]);
  }
  if (configuration.hasOwnProperty('primary_color') === true) {
    primaryColor = configuration.primary_color;
    localStorage.setItem(6, primaryColor);
  }
  if (configuration.hasOwnProperty('secondary_color') === true) {
    secondaryColor = configuration.secondary_color;
    localStorage.setItem(7, secondaryColor);
  }
  if (configuration.hasOwnProperty('background_color') === true) {
    backgroundColor = configuration.background_color;
    localStorage.setItem(8, backgroundColor);
  }
  if (configuration.hasOwnProperty('bases_display') === true) {
    basesDisplay = configuration.bases_display;
    localStorage.setItem(9, basesDisplay);
  }
  sendSettings();
  newGameDataRequest();
}

// Fucntion to send initial settings/processes to watch and load data
function initializeData(){
  loadSettings();
  newGameDataRequest();
}

// Function to get the current server
function initializeServer(){
  var url = 'http://lookup.cloud.chs.network/index.php?application=pebble';
  var method = 'GET';
  // Create the request
  var request = new XMLHttpRequest();
  
  // Specify the callback for when the request is completed
  request.onload = function() {
    // The request was successfully completed!
    var data = JSON.parse(this.responseText);
    server = data.server;
    initializeData();
  };
  // Send the request
  request.open(method, url);
  request.send();
}

// Called when JS is ready
Pebble.addEventListener("ready", function(e) {
  initializeServer();
});
												
// Called when incoming message from the Pebble is received
Pebble.addEventListener("appmessage", function(e) {
  var type = parseInt(e.payload.TYPE);
  if(type == 1){
    newGameDataRequest();
  } else if(type == 2) {
    loadSettings();
  }
});

Pebble.addEventListener('webviewclosed', function(e) {
  // Decode the user's preferences
  var configuration = JSON.parse(decodeURIComponent(e.response));
  storeSettings(configuration);
});

Pebble.addEventListener('showConfiguration', function() {
  loadSettings();
  var watch = Pebble.getActiveWatchInfo ? Pebble.getActiveWatchInfo() : null;
  var url = 'http://pebble.phl.chs.network/mlb/config/config-3.php?favorite-team=' + favoriteTeam + '&refresh-off=' + refreshTime[0] + '&refresh-game=' + refreshTime[1] + '&shake-enabled=' + shakeEnabled + '&shake-time=' + shakeTime + '&bases-display=' + basesDisplay + '&platform=' + watch.platform;
  Pebble.openURL(url);
});




