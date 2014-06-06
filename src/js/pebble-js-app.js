var CURRENT_VERSION = 20140530.01;
var CACHE_EXPIRY = 1800000;

var LOCATION_EXPIRY = 1200000;
var LOCATION_TIMEOUT = 30000;

var SETTING_DEFAULT_POSTAL_CODE = "PostalCode";
var SETTING_DEFAULT_CITY = "DefaultCity";
var SETTING_DEFAULT_COUNTRY = "DefaultCountry";
var SETTING_DEFAULT_UNIT = "DefaultUnit";
var SETTING_FORCE_LOCATION = "ForceLocation";

var PROXY_SERVICE_URL = "http://pbmovies.orilogbon.me/proxy/";
//var PROXY_SERVICE_URL = "http://192.168.0.9/pbmovies/proxy/";

var MIN_SPLASH_TIME = 500;
var MAX_DATA_LENGTH = 240;

var DELIMETER_FIELD = "|";
var DELIMETER_RECORD = "\t";

var MAX_PAGES = 10;

var DISTANCE_UNIT_KM = "km";
var DISTANCE_UNIT_MILES = "mi";
var DISTANCE_MILE_IN_M = 0.000621371;
var DISTANCE_KM_IN_M = 0.001;

var PRELOAD_WAIT_TIME = 2000;

var KEY_PRELOAD = "preloadData13";
var KEY_DEVICE_ID = "deviceId";
var KEY_SECRET_KEY = "secretKey";
var DAYS_TO_BROWSE = 5;

var showtimeTypeMask = {
    'digital': 0,
    'digital 3D': 1,
    'IMAX': 2,
    'd': 0,
    '3d': 1,
    'i': 2
};

var pebbleMessagesIn = {
    initFailed: 0
    , startApp: 1
    , connectionError: 2
    , movies: 3
    , theatres: 4
    , theatreMovies: 5
    , movieTheatres: 6
    , showtimes: 7
    , noData: 8
    , qrCode: 9
};

var pebbleMessagesOut = {
    init: 0,
    getMovies: 1,
    getTheatres: 2,
    getTheatreMovies: 3,
    getMovieTheatres: 4
    , getShowtimes: 5
    , getQrCode: 6
};


/**
 * 
 * @param {function} initDoneCallback description
 * @returns {PBMovies.service}
 */
var PBMovies = function(initDoneCallback) {
    var locationInfo = {}, secretKey, deviceId;
    var locationOptions = {"timeout": LOCATION_TIMEOUT, "maximumAge": LOCATION_EXPIRY};
    var currentDate = dateYmd();
    var dateOffset = 0;
    var PostMethods = ["register"];
    var lastPbMsgIn;
    var isPreloading = false;
    //called on successful location detection
    var locationSuccess = function(pos) {
        var coordinates = pos.coords;
        locationInfo.latlng = coordinates.latitude + "," + coordinates.longitude;
        //fetchWeather(coordinates.latitude, coordinates.longitude);
        locationSet();
    };

    var isset = function(str) {
        return str && (str.length);
    };
//called on error
    var locationError = function(err) {
        console.warn('location error (' + err.code + '): ' + err.message);
        if (isset(locationInfo.postalCode) && (isset(locationInfo.city) || isset(locationInfo.postalCode))) {
            locationSet();
        } else {
            Pebble.sendAppMessage({
                "code": pebbleMessagesIn.initFailed,
                "message": "Location Unavailable, check settings"
            });
        }
    };

    var locationSet = function() {
        console.log("Location is set" + JSON.stringify(locationInfo));
    };


    var _initRegistration = function() {
        secretKey = service.get(KEY_SECRET_KEY);
        deviceId = service.get(KEY_DEVICE_ID);
        if (!secretKey || !deviceId) {
            register();
        } else {
            registerDone();
        }
    };

    var register = function() {
        var deviceUUID = Pebble.getAccountToken();
        //console.log("UUID: " + deviceUUID);
        service.proxy('register', {'device_uuid': deviceUUID}, function(resp) {
            service.store(KEY_SECRET_KEY, secretKey = resp.device.secret_key);
            service.store(KEY_DEVICE_ID, deviceId = resp.device.id);
            registerDone();
        }, function(xhr) {
            //console.log("Response: " + xhr.responseText);
            Pebble.sendAppMessage({
                "status": 0,
                "message": "Connection error"
            });
        });
    };

    var registerDone = function() {
        console.log("Registered: " + deviceId);
        initDoneCallback();
        setTimeout(preload, 700);
    };

    var preloadRetries = 0;
    var preload = function(loadCb, errorCb) {
        var cachedData = service.isCached(KEY_PRELOAD);
        var preloadFailed = function(xhr) {
            isPreloading = false;
            if (++preloadRetries > 4) {
                preloadRetries = 0;
                if (errorCb) {
                    errorCb();
                }
            } else {
                setTimeout(function() {
                    preload(loadCb, errorCb);
                }, PRELOAD_WAIT_TIME);
            }
            console.log(xhr.responseText);
        };

        //is caching or precached
        if (!cachedData && !isPreloading) {
            isPreloading = true;
            service.proxy('preload11', {'tries': preloadRetries},
            function(response) {
                if (response.data && typeof response.data === "object") {
                    //console.log("Cachhing data");
                    service.cache(KEY_PRELOAD, response.data);
                }
                //alert
                if (response.version && response.version > CURRENT_VERSION) {
                    var lastUpdateAlert = service.get("lastUpdateAlert", true, {
                        'version': 0,
                        'time': 0
                    });
                    //show alert ?
                    if (lastUpdateAlert.version < response.version ||
                            currentTimeInMs() - lastUpdateAlert.time >= 86400000) {
                        Pebble.showSimpleNotificationOnPebble("Update Available", "A new version Pebble Movies has been published, visit Pebble App store to update!");
                        service.store("lastUpdateAlert", {'version': response.version, 'time': currentTimeInMs()});
                    }
                }
                //set preloading
                isPreloading = false;
                preloadRetries = 0;
                if (loadCb) {
                    loadCb();
                }
                //console.log("data preloaded:" + JSON.stringify(data));
            }, preloadFailed);
        }
    };

    var findIdsInList = function(list, ids, idIdx) {
        idIdx = idIdx ? idIdx : 0;
        var result = [];
        if (list && list.length) {
            for (var i = 0; i < list.length; i++) {
                if (ids.indexOf(list[i][idIdx]) > -1) {
                    result.push(list[i]);
                }
            }
        }
        return result;
    };

    var findIdInList = function(list, id, idIdx) {
        //var result = null;
        idIdx = idIdx ? idIdx : 0;
        if (list && list.length) {
            for (var i = 0; i < list.length; i++) {
                if (list[i][idIdx] == id) {
                    return list[i];
                }
            }
        }
        return null;
    };

    var messageHandler = {
        init: function() {
            initFunction();
        },
        getMovies: function() {
            messageHandler.checkPreloaded(function(data) {
                var movies = data.movies;
                if (movies && movies.length > 0) {
                    messageHandler.sendData(pebbleMessagesIn.movies, movieUtils.convertToData(movies), 1, 0, 0);
                } else {
                    messageHandler.sendNoData("No movies at the moment");
                }
            });
        },
        getMovieTheatres: function(dataIn) {
            messageHandler.checkPreloaded(function(data) {
                var movieId = dataIn.movieId;
                var movie = findIdInList(data.movies, movieId);
                //theatres is index 7
                var theatres = movie ? findIdsInList(data.theatres, movie[7]) : [];
                if (theatres.length > 0) {
                    messageHandler.sendData(pebbleMessagesIn.movieTheatres, theatreUtils.convertToData(theatres));
                } else {
                    messageHandler.sendNoData("No theatres near you");
                }
            });
        },
        getTheatres: function() {
            messageHandler.checkPreloaded(function(data) {
                var theatres = data.theatres;
                if (theatres && theatres.length > 0) {
                    messageHandler.sendData(pebbleMessagesIn.theatres, theatreUtils.convertToData(theatres));
                } else {
                    messageHandler.sendNoData("No theatres near you");
                }
            });
        },
        getTheatreMovies: function(dataIn) {
            messageHandler.checkPreloaded(function(data) {
                var theatreId = dataIn.theatreId;
                var theatre = findIdInList(data.theatres, theatreId);
                //console.log("T: "+JSON.stringify(theatre)+ " Movies: "+ JSON.stringify(theatre[4]));
                var movies = theatre ? findIdsInList(data.movies, theatre[4], 0) : [];
                //console.log("TMovies: "+JSON.stringify(movies));
                if (movies.length > 0) {
                    messageHandler.sendData(pebbleMessagesIn.theatreMovies, movieUtils.convertToData(movies));
                } else {
                    messageHandler.sendNoData("No movies at the moment");
                }
            });
        },
        checkPreloaded: function(callback) {

            var data = service.isCached(KEY_PRELOAD);
            if (isPreloading) {//if preloading, then wait
                setTimeout(function() {
                    messageHandler.checkPreloaded(callback);
                }, PRELOAD_WAIT_TIME);
            } else if (!data) {//not preloading, and no data, try to preload
                preload(function() {
                    messageHandler.checkPreloaded(callback);
                }, messageHandler.handleErrors);
            } else {
                callback(data); //data preloaded, call callback
            }
        },
        getShowtimes: function(dataIn) {
            messageHandler.checkPreloaded(function(data) {
                var key = dataIn.theatreId + "." + dataIn.movieId;
                //console.log("Showtime Key: " + key);
                var showtimes = data.showtimes ? data.showtimes[key] : [];
                showtimeUtils.processShowtimes(showtimes || []);
            });
        },
        getQrCode: function(dataIn) {
            var showtimeId = dataIn.showtimeId;
            //console.log("In Here for getQrCode");
            var resourceUrl = service.proxy('qr', {"showtime_id": showtimeId}, null, messageHandler.handleErrors, true);
            //console.log("Resource URL: " + resourceUrl);
            downloadBinaryResource(resourceUrl, function(bytes) {
                transferImageBytes(bytes, MAX_DATA_LENGTH,
                        function() {
                            console.log("Done!");
                            //transferInProgress = false;
                        },
                        function(e) {
                            //console.log("Failed! " + e);
                            //transferInProgress = false;
                        }
                );
            }, function(e) {
                console.log(e);
            });
        },
        handleErrors: function(err) {
            //console.log("Errors occured while getting data :" + JSON.stringify(err));
            Pebble.sendAppMessage({
                "code": pebbleMessagesIn.connectionError,
                "message": "Connection Error"
            });
        },
        truncateDate: function(data) {
            var maxDataLength = MAX_DATA_LENGTH * MAX_PAGES;
            if (data.length > maxDataLength) {
                var tmp = data.substring(0, maxDataLength);
                data = tmp.substring(0, tmp.lastIndexOf(DELIMETER_RECORD));
            }
            return data;
        },
        sendData: function(msgCode, data, currentPage, raw, retries) {
            lastPbMsgIn = msgCode;
            if (!retries) {
                retries = 0;
            }
            data = messageHandler.truncateDate(data);
            if (!currentPage)
                currentPage = 1;

            var totalPages = Math.ceil(data.length / MAX_DATA_LENGTH);
            //console.log("Sending page " + currentPage + " of " + totalPages);
            var offset = (currentPage - 1) * MAX_DATA_LENGTH;
            var outData = {
                "code": msgCode
                , "data": data.substring(offset, MAX_DATA_LENGTH * currentPage)
                , "page": currentPage
                , "totalPages": totalPages
            };
            console.log("Sending page " + currentPage + " of " + totalPages + " Length = " + outData.data.length);

            //console.log("Out data" + JSON.stringify(outData));
            Pebble.sendAppMessage(outData, function(e) {
                retries = 0;
                //console.log("Delivered");
                if (currentPage < totalPages && currentPage < MAX_PAGES) {
                    messageHandler.sendData(msgCode, data, ++currentPage, raw, retries);
                }

            }, function(e) {
                if (retries++ < 3) {
                    console.log("Retrying... [" + retries + "] previous failed: " + JSON.stringify(e));
                    messageHandler.sendData(msgCode, data, ++currentPage, raw, retries);
                } else {
                    console.log("Failed...");
                }
            });

        },
        sendNoData: function(msg) {
            Pebble.sendAppMessage({
                "code": pebbleMessagesIn.noData,
                "message": msg
            });
        }
    };

    var clean = function(record){
        for(var i in record){
            if(typeof record[i] === "string" && (record[i].indexOf(DELIMETER_FIELD) > -1 || record[i].indexOf(DELIMETER_RECORD) > -1)){
                record[i] = record[i].replace(DELIMETER_FIELD, "/").replace(DELIMETER_RECORD, " ").trim();
            }
                
        }
        return record;
    };

    var showtimeUtils = {
        //"id", "time", "type", "link"
        processShowtimes: function(showtimes) {
            var records = [];
            for (var i = 0; i < showtimes.length; i++) {
                var showtime = [];
                var time = showtimeUtils.formatTime(currentDate, showtimes[i][1]);
                showtime.push(showtimes[i][0]);
                showtime.push(showtimeTypeMask[showtimes[i][2]]);
                showtime.push(time);
                showtime.push(showtimes[i][3] ? 1 : 0);
                records.push(showtime.join(DELIMETER_FIELD));
            }
            //bug 
            if (records.length) {
                //console.log("Showtimes: " + JSON.stringify(records));
                messageHandler.sendData(pebbleMessagesIn.showtimes, records.join(DELIMETER_RECORD));
            } else {
                messageHandler.sendNoData("No showtimes");
            }
        },
        formatTime: function(showdate, showtime) {
            var arr = (showdate + " " + showtime).split(/[- :T]/);
//And use the constructor with 6 args
            var d = new Date(arr[0], arr[1] - 1, arr[2], arr[3], arr[4], 0);
            //var d = new Date(showdate + " " + showtime);
            var min = d.getMinutes();
            var hours = d.getHours();
            var hoursHp = hours % 12 || 12;
            return (hoursHp > 9 ? hoursHp : "0" + hoursHp) + ":"
                    + (min > 9 ? min : "0" + min)
                    + (hours >= 12 ? "PM" : "AM");

        }
    };
    var theatreUtils = {
        formatDistance: function(dist) {
            var distNum = parseFloat(dist);
            if (distNum < 1 || isNaN(distNum)) {
                return " ";
            }
            var prefUnit = service.get(SETTING_DEFAULT_UNIT, false, DISTANCE_UNIT_KM);
            var conversion = prefUnit === DISTANCE_UNIT_KM ? DISTANCE_KM_IN_M : DISTANCE_MILE_IN_M;
            return Number(conversion * distNum).toPrecision(2) + prefUnit;
        },
        convertToData: function(theatres) {
            var theatre, records = [];
            sort_in_place(theatres, 3, function(t1, t2) {
                var dist1 = parseInt(t1[3]);
                var dist2 = parseInt(t2[3]);
                if (dist1 === dist2) {
                    return 0;
                }
                dist1 = dist1 < 0 ? 10000000000 : dist1;
                dist2 = dist2 < 0 ? 10000000000 : dist2;
                return dist1 > dist2 ? 1 : -1;
            });

            for (var i = 0; i < theatres.length; i++) {
                //"id,name,address,distance_m"
                theatre = theatres[i];
                theatre[3] = theatreUtils.formatDistance(theatre[3]);
                theatre = clean(theatre);
                records.push(theatre.join(DELIMETER_FIELD));
            }
            return records.join(DELIMETER_RECORD);
        }
    };

    var movieUtils = {
        convertToData: function(movies) {
            sort_in_place(movies, 'title');
            var movie, records = [];
            for (var i = 0; i < movies.length; i++) {
                //id,title,genre,user_rating,rated,critic_rating,runtime                    
                try {
                    movie = movies[i];
                    movie[2] = movie[2] || " ";
                    movie[3] = Number(movie[3] * 5).toPrecision(2) + "/5";
                    movie[4] = movie[4] ? movie[4] + "" : "NR";
                    movie[4] = !movie[4].trim().length ? "NR" : movie[4];
                    movie[4] = movie[4].replace(/(Not Rated)|(Unrated)/i, "NR");
                    movie[5] = Math.min(Math.round(parseFloat(movie[5]) * 100), 99);
                    movie = clean(movie);
                } catch (e) {
                    console.log("Error " + movie[4] + "__" + e.message);
                }
                records.push(movie.join(DELIMETER_FIELD));
            }

            return records.join(DELIMETER_RECORD);
        }
    };

    var service = {
        unStore: function(key) {
            if (window.localStorage) {
                delete window.localStorage[key];
            }
        },
        isStored: function(key) {
            return window.localStorage && window.localStorage[key] !== undefined;
        },
        store: function(key, val, asObject) {
            try {
                window.localStorage.setItem(key, asObject ? JSON.stringify(val) : val);
            } catch (e) {
                console.log("Error occured while saving item");
            }
        },
        get: function(key, asObject, defaultValue) {
            try {
                if (window.localStorage && window.localStorage[key]) {
                    return asObject ? JSON.parse(window.localStorage.getItem(key)) : window.localStorage.getItem(key);
                }
            } catch (e) {
                console.log("Storage Object error");
            }
            return defaultValue ? defaultValue : null;
        },
        proxy: function(command, data, successCallback, errorCallback, urlOnly) {
            var method = PostMethods.indexOf(command) > -1 ? "POST" : "GET";
            var urlData = {token: service.signRequest(), 'date': currentDate, 'version': CURRENT_VERSION, 'dateOffset': dateOffset};
            var forceLocation = parseInt(movieService.get(SETTING_FORCE_LOCATION, false, "0"));
            for (i in locationInfo) {
                if (locationInfo.hasOwnProperty(i) && locationInfo[i]) {
                    if (!(i === 'latlng' && forceLocation)) {
                        urlData[i] = locationInfo[i];
                    }
                }
            }

            var url = PROXY_SERVICE_URL + command + "?" + serializeData(urlData);
            var reqData = method === 'POST' ? data : null;
            if (method === 'GET' && data) {
                url += "&" + serializeData(data);
            }
            if (urlOnly) {
                return url;
            }
            console.log("Making proxy command for- " + command.toUpperCase() + " Method");
            //console.log("URL: " + url);
            return makeRequest(url, method, reqData, successCallback, errorCallback);
        },
        signRequest: function() {
            //token requestId|deviceId|sign=sha1(requestId.deviceId.secretKey)
            var requestId = currentTimeInMs();
            return [requestId, deviceId, sha1(requestId + deviceId + secretKey)].join("|");
        },
        postRequest: function(url, data, successHandler, errorHandler) {
            return makeRequest(url, data, 'POST', successHandler, errorHandler);
        },
        getRequest: function(url, successHandler, errorHandler) {
            return makeRequest(url, 'GET', successHandler, errorHandler);
        },
        cacheKey: function(dataKey, offset) {
            return 'cache_' + dataKey + "_" + dateYmdWithOffset(offset !== undefined ? offset : dateOffset);
        },
        isCached: function(dataKey, ignoreExpiry) {
            var k = service.cacheKey(dataKey);
            if (service.isStored(k)) {
                var cached = service.get(k, true);
                try {
                    if (ignoreExpiry || (cached && cached.expiry > currentTimeInMs())) {
                        console.log("Cache HIT");
                        return cached.data;
                    }
                } catch (e) {
                    console.log("Error occured in cache checking...");
                }

            }
            return null;
        },
        cache: function(dataKey, data, validity) {
            var k = service.cacheKey(dataKey);
            validity = validity ? validity : CACHE_EXPIRY;
            return service.store(k, {expiry: currentTimeInMs() + validity, 'data': data}, true);
        },
        uncache: function(dataKey, offset) {
            var k = service.cacheKey(dataKey, offset);
            service.unStore(k);
        },
        serviceError: function(cacheKey, xhr, onSuccess, onError) {
            var cached = service.isCached(cacheKey, true);
            if (cached) {
                return onSuccess(cached);
            }
            if (onError) {
                try {
                    onError(JSON.parse(xhr.responseText).errors);
                } catch (e) {
                }
            }
        },
        handleMessage: function(payload) {
            var msgCode = parseInt(payload.code);

            if (payload.dateOffset !== undefined) {
                dateOffset = payload.dateOffset;
            }
            //console.log("Handling message... "+JSON.stringify(payload));
            for (var i in pebbleMessagesOut) {
                if (pebbleMessagesOut[i] === msgCode) {
                    if (messageHandler.hasOwnProperty(i)) {
                        //console.log("Handler found ");
                        return messageHandler[i](payload);
                    } else {
                        console.log("Message handler does not have method for: " + i);
                        return null;
                    }
                }
            }
            console.log("can't handle message code: " + msgCode);
        },
        loadLoactionInfo: function() {
            locationInfo.postalCode = service.get(SETTING_DEFAULT_POSTAL_CODE, false, "");
            locationInfo.city = service.get(SETTING_DEFAULT_CITY, false, "");
            locationInfo.country = service.get(SETTING_DEFAULT_COUNTRY, false, "");
        }

    };

    var init = function() {
        //var locationInfo = service.get('location', true);
        service.loadLoactionInfo();
        window.navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions);
        _initRegistration();
    };


    init();
    return service;
};

//the pebble app itself
/**
 * 
 * @type PBMovies.service
 */
var movieService;

var initFunction = function(sends) {
    //var timeSinceLaunch, timeStarted = currentTimeInMs();
    sends = sends || 1;
    movieService = new PBMovies(function() {
        //timeSinceLaunch = currentTimeInMs() - timeStarted;
        setTimeout(function() {
            Pebble.sendAppMessage({
                "code": pebbleMessagesIn.startApp
            }, function(e) {
                console.log("App started!");
            }, function(e) {
                console.log("Init failed: ");
                if (sends++ < 4) {
                    initFunction(sends);
                }
            });
        }, MIN_SPLASH_TIME);
    });
    //movieService.unStore("secretKey");
};

Pebble.addEventListener("ready", function(e) {
    initFunction();
});


Pebble.addEventListener("appmessage",
        function(e) {
            //console.log("Received message: " + JSON.stringify(e.payload));
            if (movieService) {
                movieService.handleMessage(e.payload);
            }
        }
);


Pebble.addEventListener("webviewclosed", function(e) {
    //console.log("configuration closed");
    // webview closed
    var options = JSON.parse(decodeURIComponent(e.response));
    var keys = [SETTING_DEFAULT_CITY, SETTING_DEFAULT_COUNTRY, SETTING_DEFAULT_POSTAL_CODE, SETTING_DEFAULT_UNIT, SETTING_FORCE_LOCATION];
    var changed = false;
    if (options) {
        for (var i = 0; i < keys.length; i++) {
            if (keys[i] in options && options[keys[i]] !== null) {
                changed = changed || options[keys[i]] != movieService.get(keys[i]);
                movieService.store(keys[i], options[keys[i]]);
            }
        }
    }

    if (changed) {
        for (var i = 0; i < DAYS_TO_BROWSE; i++) {
            movieService.uncache(KEY_PRELOAD, i);
        }
        movieService.loadLoactionInfo();
    }

    console.log("Options = " + JSON.stringify(options));
});


Pebble.addEventListener("showConfiguration", function() {
    console.log("showing configuration");
    var proxyUrl = movieService.proxy('settings', {
        'unit': movieService.get(SETTING_DEFAULT_UNIT, false, DISTANCE_UNIT_KM)
        , 'forceLocation': movieService.get(SETTING_FORCE_LOCATION, false, "0")
    }, null, null, true);
    console.log("opening settings url: " + proxyUrl);
    Pebble.openURL(proxyUrl);
});


function serializeData(data) {
    if (data) {
        if (typeof data !== "object") {
            return data;
        }

        var parts = [];
        for (i in data) {
            if (data.hasOwnProperty(i)) {
                parts.push(i + "=" + encodeURIComponent(data[i]));
            }
        }
        return parts.join("&");
    }
    return '';
}


function currentTimeInMs() {
    return Date.now();
}

function transferImageBytes(bytes, chunkSize, successCb, failureCb) {
    var retries = 0;

    success = function() {
        console.log("Success cb=" + successCb);
        if (successCb !== undefined) {
            successCb();
        }
    };
    failure = function(e) {
        console.log("Failure cb=" + failureCb);
        if (failureCb !== undefined) {
            failureCb(e);
        }
    };

    // This function sends chunks of data.
    sendChunk = function(start) {
        var txbuf = bytes.slice(start, start + chunkSize);

        console.log("Sending " + txbuf.length + " bytes - starting at offset " + start);
        var page = Math.round(start / chunkSize) + 1;
        var totalPages = Math.ceil(bytes.length / chunkSize);
        Pebble.sendAppMessage({
            "data": txbuf,
            "code": pebbleMessagesIn.qrCode,
            "page": page,
            "totalPages": totalPages
        },
        function(e) {
            // If there is more data to send - send it.
            if (bytes.length > start + chunkSize) {
                sendChunk(start + chunkSize);
            }
// Otherwise we are done sending. Send closing message.
//            else {
//                Pebble.sendAppMessage({"NETIMAGE_END": "done"}, success, failure);
//            }
        },
                // Failed to send message - Retry a few times.
                        function(e) {
                            if (retries++ < 3) {
                                console.log("Got a nack for chunk #" + start + " - Retry...");
                                sendChunk(start);
                            }
                            else {
                                failure(e);
                            }
                        }
                );
            };

    sendChunk(0);
    // Let the pebble app know how much data we want to send.
//  Pebble.sendAppMessage({"NETIMAGE_BEGIN": bytes.length },
//    function (e) {
//      // success - start sending
//      sendChunk(0);
//    }, failure);

}

function downloadBinaryResource(imageURL, callback, errorCallback) {
    var req = new XMLHttpRequest();
    req.open("GET", imageURL, true);
    req.responseType = "arraybuffer";
    req.onreadystatechange = function(e) {
        if (req.readyState === 4) {
            console.log("loaded");
            var buf = req.response;
            if (req.status === 200 && buf) {
                var byteArray = new Uint8Array(buf);
                var arr = [];
                for (var i = 0; i < byteArray.byteLength; i++) {
                    arr.push(byteArray[i]);
                }

                console.log("Received image with " + byteArray.length + " bytes.");
                callback(arr);
            }
            else {
                errorCallback("Request status is " + req.status);
            }
        }
    };
    req.onerror = function(e) {
        errorCallback(e);
    };
    req.send(null);
}

/**
 * 
 * @param {string} url
 * @param {type} method
 * @param {type} data
 * @param {type} successHandler
 * @param {type} errorHandler
 * @returns {undefined}
 */
var makeRequest = function(url, method, data, successHandler, errorHandler) {
    var response, startTime = currentTimeInMs();
    var xhr = new XMLHttpRequest();
    xhr.open(method || 'GET', url, true);
    data = data ? serializeData(data) : null;
    if (method === "POST") {
        xhr.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
        if (data) {
            xhr.setRequestHeader("Content-length", data.length);
        }
        xhr.setRequestHeader("Connection", "close");
    }

    xhr.onreadystatechange = function(e) {
        if (xhr.readyState === 4) {
            console.log("Request completed in " + (currentTimeInMs() - startTime) + "ms");
            console.log("HTTP Status: " + xhr.status);
            if (xhr.status === 200) {
                //console.log(xhr.responseText);
                if (successHandler) {
                    //console.log("Received Mime: "+xhr.getResponseHeader('content-type'));
                    if (xhr.getResponseHeader('content-type').match(/image|octet|stream/)) {
                        response = xhr.responseText;
                    } else {
                        response = JSON.parse(xhr.responseText);
                    }
                    successHandler(response, xhr);
                }
            } else {
                console.log("Error");
                if (errorHandler) {
                    errorHandler(xhr);
                }
            }
        }
    };
    xhr.send(data);
};

function objectValues(obj, exclude) {
    var op = [];
    var toExclude = exclude || [];
    for (var key in obj) {
        if (obj.hasOwnProperty(key) && toExclude.indexOf(key) < 0) {
            op.push(obj[key]);
        }
    }
    return op;
}
/**
 * 
 * @param {Array} objects
 * @param {string} key
 * @param {function} compCb
 * @returns {undefined}
 */
function sort_in_place(objects, key, compCb) {
    var tmp, isGreater;
    for (var i = 0; i < objects.length; i++) {
        for (var j = i + 1; j < objects.length; j++) {
            isGreater = compCb ? (compCb(objects[i], objects[j]) > 0) :
                    (objects[i][key] > objects[j][key]);
            if (isGreater) {
                tmp = objects[i];
                objects[i] = objects[j];
                objects[j] = tmp;
            }
        }
    }
}

function dateYmd(ts) {
    var t = ts ? new Date(ts) : new Date();
    var dd = t.getDate() > 9 ? t.getDate() : "0" + t.getDate();
    var mnt = t.getMonth() + 1;
    var mm = mnt > 9 ? mnt : "0" + mnt;
    return t.getFullYear() + "-" + mm + "-" + dd;
}
function dateYmdWithOffset(o) {
    return dateYmd(Date.now() + o * 86400000);
}
function utf8_encode(a) {
    if (a === null || typeof a === 'undefined') {
        return'';
    }
    var b = (a + '');
    var c = '', start, end, stringl = 0;
    start = end = 0;
    stringl = b.length;
    for (var n = 0; n < stringl; n++) {
        var d = b.charCodeAt(n);
        var e = null;
        if (d < 128) {
            end++;
        } else if (d > 127 && d < 2048) {
            e = String.fromCharCode((d >> 6) | 192, (d & 63) | 128);
        } else if ((d & 0xF800) != 0xD800) {
            e = String.fromCharCode((d >> 12) | 224, ((d >> 6) & 63) | 128, (d & 63) | 128);
        } else {
            if ((d & 0xFC00) != 0xD800) {
                throw new RangeError('Unmatched trail surrogate at ' + n);
            }
            var f = b.charCodeAt(++n);
            if ((f & 0xFC00) != 0xDC00) {
                throw new RangeError('Unmatched lead surrogate at ' + (n - 1));
            }
            d = ((d & 0x3FF) << 10) + (f & 0x3FF) + 0x10000;
            e = String.fromCharCode((d >> 18) | 240, ((d >> 12) & 63) | 128, ((d >> 6) & 63) | 128, (d & 63) | 128);
        }
        if (e !== null) {
            if (end > start) {
                c += b.slice(start, end);
            }
            c += e;
            start = end = n + 1;
        }
    }
    if (end > start) {
        c += b.slice(start, stringl);
    }
    return c;
}

function sha1(c) {
    var d = function(n, s) {
        var a = (n << s) | (n >>> (32 - s));
        return a;
    };
    var e = function(a) {
        var b = '';
        var i;
        var v;
        for (i = 7; i >= 0; i--) {
            v = (a >>> (i * 4)) & 0x0f;
            b += v.toString(16);
        }
        return b;
    };
    var f;
    var i, j;
    var W = new Array(80);
    var g = 0x67452301;
    var h = 0xEFCDAB89;
    var k = 0x98BADCFE;
    var l = 0x10325476;
    var m = 0xC3D2E1F0;
    var A, B, C, D, E;
    var o;
    c = this.utf8_encode(c);
    var p = c.length;
    var q = [];
    for (i = 0; i < p - 3; i += 4) {
        j = c.charCodeAt(i) << 24 | c.charCodeAt(i + 1) << 16 | c.charCodeAt(i + 2) << 8 | c.charCodeAt(i + 3);
        q.push(j);
    }
    switch (p % 4) {
        case 0:
            i = 0x080000000;
            break;
        case 1:
            i = c.charCodeAt(p - 1) << 24 | 0x0800000;
            break;
        case 2:
            i = c.charCodeAt(p - 2) << 24 | c.charCodeAt(p - 1) << 16 | 0x08000;
            break;
        case 3:
            i = c.charCodeAt(p - 3) << 24 | c.charCodeAt(p - 2) << 16 | c.charCodeAt(p - 1) << 8 | 0x80;
            break
    }
    q.push(i);
    while ((q.length % 16) != 14) {
        q.push(0);
    }
    q.push(p >>> 29);
    q.push((p << 3) & 0x0ffffffff);
    for (f = 0; f < q.length; f += 16) {
        for (i = 0; i < 16; i++) {
            W[i] = q[f + i];
        }
        for (i = 16; i <= 79; i++) {
            W[i] = d(W[i - 3] ^ W[i - 8] ^ W[i - 14] ^ W[i - 16], 1);
        }
        A = g;
        B = h;
        C = k;
        D = l;
        E = m;
        for (i = 0; i <= 19; i++) {
            o = (d(A, 5) + ((B & C) | (~B & D)) + E + W[i] + 0x5A827999) & 0x0ffffffff;
            E = D;
            D = C;
            C = d(B, 30);
            B = A;
            A = o;
        }
        for (i = 20; i <= 39; i++) {
            o = (d(A, 5) + (B ^ C ^ D) + E + W[i] + 0x6ED9EBA1) & 0x0ffffffff;
            E = D;
            D = C;
            C = d(B, 30);
            B = A;
            A = o;
        }
        for (i = 40; i <= 59; i++) {
            o = (d(A, 5) + ((B & C) | (B & D) | (C & D)) + E + W[i] + 0x8F1BBCDC) & 0x0ffffffff;
            E = D;
            D = C;
            C = d(B, 30);
            B = A;
            A = o;
        }
        for (i = 60; i <= 79; i++) {
            o = (d(A, 5) + (B ^ C ^ D) + E + W[i] + 0xCA62C1D6) & 0x0ffffffff;
            E = D;
            D = C;
            C = d(B, 30);
            B = A;
            A = o;
        }
        g = (g + A) & 0x0ffffffff;
        h = (h + B) & 0x0ffffffff;
        k = (k + C) & 0x0ffffffff;
        l = (l + D) & 0x0ffffffff;
        m = (m + E) & 0x0ffffffff;
    }
    o = e(g) + e(h) + e(k) + e(l) + e(m);
    return o.toLowerCase();
}