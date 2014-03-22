var CACHE_EXPIRY = 1800000;

var LOCATION_EXPIRY = 1200000;
var LOCATION_TIMEOUT = 30000;

var SETTING_DEFAULT_POSTAL_CODE = "PostalCode";
var SETTING_DEFAULT_CITY = "DefaultCity";
var SETTING_DEFAULT_COUNTRY = "DefaultCountry";
var SETTING_DEFAULT_UNIT = "DefaultUnit";

var PROXY_SERVICE_URL = "http://pbmovies.orilogbon.me/proxy/";

var MIN_SPLASH_TIME = 2000;
var MAX_DATA_LENGTH = 480;

var DELIMETER_FIELD = "|";
var DELIMETER_RECORD = "\t";

var MAX_PAGES = 5;
var MSG_INTERVAL = 2000;

var DISTANCE_UNIT_KM = "km";
var DISTANCE_UNIT_MILES = "mi";
var DISTANCE_MILE_IN_M = 0.000621371;
var DISTANCE_KM_IN_M = 0.001;

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
};

var pebbleMessagesOut = {
    init: 0,
    getMovies: 1,
    getTheatres: 2,
    getTheatreMovies: 3,
    getMovieTheatres: 4
    , getShowtimes: 5
};
//console.log("Is this working");
/**
 * 
 * @returns {PBMovies.service}
 */
var PBMovies = function(initDoneCallback) {
    var locationInfo = {}, secretKey, deviceId, appConfig;
    var locationOptions = {"timeout": LOCATION_TIMEOUT, "maximumAge": LOCATION_EXPIRY};
    var currentDate = dateYmd();
    var PostMethods = ["register"];
    //called on successful location detection
    var locationSuccess = function(pos) {
        var coordinates = pos.coords;
        locationInfo.latlng = coordinates.latitude + "," + coordinates.longitude;
        //fetchWeather(coordinates.latitude, coordinates.longitude);
        locationSet();
    };

//called on error
    var locationError = function(err) {
        console.warn('location error (' + err.code + '): ' + err.message);
        if (appConfig[SETTING_DEFAULT_COUNTRY] && (appConfig[SETTING_DEFAULT_CITY] || appConfig[SETTING_DEFAULT_POSTAL_CODE])) {
            locationInfo.postalCode = appConfig[SETTING_DEFAULT_POSTAL_CODE];
            locationInfo.city = appConfig[SETTING_DEFAULT_CITY];
            locationInfo.country = appConfig[SETTING_DEFAULT_COUNTRY];
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
        _initRegistration();
    };


    var init = function() {
        appConfig = service.get('appConfig');
        //var locationInfo = service.get('location', true);
        window.navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions);
    };

    var _initRegistration = function() {
        secretKey = service.get('secretKey');
        deviceId = service.get('deviceId');
        if (!secretKey || !deviceId) {
            register();
        } else {
            registerDone();
        }
    };

    var register = function() {
        var deviceUUID = Pebble.getAccountToken();
        console.log("UUID: " + deviceUUID);
        service.proxy('register', {'device_uuid': deviceUUID}, function(resp) {
            service.store('secretKey', secretKey = resp.device.secret_key);
            service.store('deviceId', deviceId = resp.device.id);
            registerDone();
        }, function(xhr) {
            console.log("Response: " + xhr.responseText);
            Pebble.sendAppMessage({
                "status": 0,
                "message": "Connection error"
            });
        });
    };

    var registerDone = function() {
        console.log("Registration Done: Device ID : " + deviceId + " \nSecretKey: " + secretKey.substring(0, 10) + "...");
        //service.isReady = true;
        initDoneCallback();
        preload();
    };

    var preload = function() {
        service.proxy('preload', null, function(data) {
            console.log("data preloaded:" + JSON.stringify(data));
        }, function(xhr) {
            console.log("Status: " + xhr.status + " Text: " + xhr.responseText);
        });
    };

    var messageHandler = {
        init: function() {
            initFunction();
        },
        getMovies: function() {
            service.getMovies(function(movies) {
                if (movies.length > 0) {
                    var movie, records = [];
                    for (var i = 0; i < movies.length; i++) {
                        //id,title,genre,user_rating,rated,critic_rating,runtime                    
                        movie = objectValues(movies[i]);
                        movie[2] = movie[2] || " "; 
                        movie[3] = Number(movie[3] * 5).toPrecision(2) + "/5";
                        movie[4] = (!movie[4] || !movie[4].trim().length) ? "NR" : movie[4];
                        movie[4] = movie[4].replace(/Not Rated/i, "NR");
                        movie[5] = Math.min(Math.round(parseFloat(movie[5])*100),99);
                        records.push(movie.join(DELIMETER_FIELD));
                    }
                    messageHandler.sendData(pebbleMessagesIn.movies, records.join(DELIMETER_RECORD));
                } else {
                    messageHandler.sendNoData("No movies at the moment");
                }
            }, messageHandler.handleErrors);
        },
        getTheatres: function() {
            service.getTheatres(function(theatres) {
                if (theatres.length > 0) {
                    var theatre, records = [];
                    for (var i = 0; i < theatres.length; i++) {
                        //"id,name,address,distance_m"
                        theatre = objectValues(theatres[i]);
                        theatre[3] = theatreUtils.formatDistance(theatre[3]);
                        records.push(theatre.join(DELIMETER_FIELD));
                    }
                    messageHandler.sendData(pebbleMessagesIn.theatres, records.join(DELIMETER_RECORD));
                } else {
                    messageHandler.sendNoData("No theatres near you");
                }
            }, messageHandler.handleErrors);
        },
        handleErrors: function(err) {
            console.log("Errors occured while getting data :" + JSON.stringify(err));
            Pebble.sendAppMessage({
                "code": pebbleMessagesIn.connectionError,
                "message": "Connection Error"
            });
        },
        sendData: function(msgCode, data, currentPage) {
            
            if(data.match(/[^\x00-\x7F]/g)){
                data = data.replace(/[^\x00-\x7F]/g, "*");
            }
            
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
            
            //console.log("Out data"+JSON.stringify(outData));
            Pebble.sendAppMessage(outData);
            if (currentPage < totalPages && currentPage < MAX_PAGES) {
                setTimeout(function() {
                    messageHandler.sendData(msgCode, data, ++currentPage);
                }, MSG_INTERVAL);
            }
        },
        sendNoData: function(msg) {
            Pebble.sendAppMessage({
                "code": pebbleMessagesIn.noData,
                "message": msg
            });
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
        }
    };

    var service = {
        unStore: function(key) {
            delete localStorage[key];
        },
        isStored: function(key) {
            return localStorage.hasOwnProperty(key);
        },
        store: function(key, val, asObject) {
            localStorage.setItem(key, asObject ? JSON.stringify(val) : val);
        },
        get: function(key, asObject, defaultValue) {
            if (localStorage.hasOwnProperty(key)) {
                return asObject ? JSON.parse(localStorage.getItem(key)) : localStorage.getItem(key);
            }
            return defaultValue ? defaultValue : null;
        },
        proxy: function(command, data, successCallback, errorCallback) {
            var method = PostMethods.indexOf(command) > -1 ? "POST" : "GET";
            var urlData = {token: service.signRequest(), 'date': currentDate};
            for (i in locationInfo) {
                if (locationInfo.hasOwnProperty(i)) {
                    urlData[i] = locationInfo[i];
                }
            }
            var url = PROXY_SERVICE_URL + command + "?" + serializeData(urlData);
            var reqData = method === 'POST' ? data : null;
            if (method === 'GET' && data) {
                url += "&" + serializeData(data);
            }
            console.log("Making proxy command: " + command + " Method");
            console.log("URL: " + url);
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
        isCached: function(dataKey, ignoreExpiry) {
            var k = 'cache_' + dataKey;
            if (service.isStored(k)) {
                var cached = service.get(k, true);
                if (ignoreExpiry || cached.expiry > currentTimeInMs()) {
                    return cached.data;
                } else {
                    service.unStore(k);
                }
            }
            return null;
        },
        cache: function(dataKey, data, validity) {
            var k = 'cache_' + dataKey;
            validity = validity ? validity : CACHE_EXPIRY;
            return service.store(k, {expiry: currentTimeInMs() + validity, 'data': data}, true);
        },
        getMovies: function(onSuccess, onError) {
            var key = "movies";
            var cached = service.isCached(key);
            if (cached) {
                return onSuccess(cached);
            }

            service.proxy('movies', null, function(resp) {
                service.cache(key, resp.movies);
                onSuccess(resp.movies);
            }, function(xhr) {
                service.serviceError(key, xhr, onSuccess, onError);
            });
        },
        getTheatres: function(onSuccess, onError) {
            var key = "theatres";
            var cached = service.isCached(key);
            if (cached) {
                return onSuccess(cached);
            }

            service.proxy('theatres', null, function(resp) {
                service.cache(key, resp.theatres);
                onSuccess(resp.theatres);
            }, function(xhr) {
                service.serviceError(key, xhr, onSuccess, onError);
            });
        },
        getMovieTheatres: function(movieId, onSuccess, onError) {
            var key = "movie_theatres_" + movieId;
            var cached = service.isCached(key);
            if (cached) {
                return onSuccess(cached);
            }

            service.proxy('movie-theatres', {movie_id: movieId}, function(resp) {
                service.cache(key, resp.movie_theatres);
                onSuccess(resp.movie_theatres);
            }, function(xhr) {
                service.serviceError(key, xhr, onSuccess, onError);
            });
        },
        getTheatreMovies: function(theatreId, onSuccess, onError) {
            var key = "theatre_movies_" + theatreId;
            var cached = service.isCached(key);
            if (cached) {
                return onSuccess(cached);
            }

            service.proxy('theatre-movies', {theatre_id: theatreId}, function(resp) {
                service.cache(key, resp.theatre_movies);
                onSuccess(resp.theatre_movies);
            }, function(xhr) {
                service.serviceError(key, xhr, onSuccess, onError);
            });
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
            for (var i in pebbleMessagesOut) {
                if (pebbleMessagesOut[i] === msgCode) {
                    if (messageHandler.hasOwnProperty(i)) {
                        return messageHandler[i](payload);
                    } else {
                        console.log("Message handler does not have method for: " + i);
                        return null;
                    }
                }
            }
            console.log("can't handle message code: " + msgCode);
        }

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
var initFunction = function() {
    var timeSinceLaunch, timeStarted = currentTimeInMs();
    movieService = new PBMovies(function() {
        timeSinceLaunch = currentTimeInMs() - timeStarted;
        setTimeout(function() {
            console.log("calling off splash screen");//
            Pebble.sendAppMessage({
                "code": pebbleMessagesIn.startApp
            });
        }, timeSinceLaunch >= MIN_SPLASH_TIME ? 10 : (MIN_SPLASH_TIME - timeSinceLaunch) + 10);
    });
};

Pebble.addEventListener("ready", function(e) {
    initFunction();
});


Pebble.addEventListener("appmessage",
        function(e) {
            console.log("Received message: " + JSON.stringify(e.payload));
            if (movieService) {
                movieService.handleMessage(e.payload);
            }
        }
);



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
    return new Date().getTime();
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
    var response;
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
            if (xhr.status === 200) {
                //console.log(xhr.responseText);
                if (successHandler) {
                    response = JSON.parse(xhr.responseText);
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

function dateYmd() {
    var t = new Date();
    var dd = t.getDate() > 9 ? t.getDate() : "0" + t.getDay();
    var mnt = t.getMonth() + 1;
    var mm = mnt > 9 ? mnt : "0" + mnt;
    return t.getFullYear() + "-" + mm + "-" + dd;
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