/* global Pebble */
(function (pebble) {

  var NUMBER_RADIX = 32;

  var CURRENT_VERSION = 20160111.02;
  var CACHE_EXPIRY = 1800000;

  var LOCATION_EXPIRY = 1200000;
  var LOCATION_TIMEOUT = 30000;

  var SETTING_DEFAULT_POSTAL_CODE = "PostalCode";
  var SETTING_DEFAULT_CITY = "DefaultCity";
  var SETTING_DEFAULT_COUNTRY = "DefaultCountry";
  var SETTING_DEFAULT_UNIT = "DefaultUnit";
  var SETTING_FORCE_LOCATION = "ForceLocation";
  var SETTING_REMINDER = "Reminder";

  var PROXY_SERVICE_URL = "http://pbmovies.orilogbon.me/proxy/";

  var MIN_SPLASH_TIME = 500;
  var MAX_DATA_LENGTH = 240;

  var DELIMETER_FIELD = "|";
  var DELIMETER_RECORD = "\t";

  var MAX_PAGES = 10;

  var DISTANCE_UNIT_KM = "km";
//var DISTANCE_UNIT_MILES = "mi";
  var DISTANCE_MILE_IN_M = 0.000621371;
  var DISTANCE_KM_IN_M = 0.001;

  var DEFAULT_REMINDER = 30;

  var PRELOAD_WAIT_TIME = 2000;

  var KEY_PRELOAD = "preloadData26";
  var KEY_DEVICE_ID = "deviceId";
  var KEY_SECRET_KEY = "secretKey";
  var DAYS_TO_BROWSE = 5;

  var FEATURE_NOTIFY_TIMELINE = "featureNotifyTimeline-1";

  var showtimeTypeMask = {
    'digital': 0,
    'digital 3D': 1,
    'IMAX': 2,
    'd': 0,
    '3d': 1,
    'i': 2
  };

  var pebbleMessagesIn = {
    initFailed: 0,
    startApp: 1,
    connectionError: 2,
    movies: 3,
    theatres: 4,
    theatreMovies: 5,
    movieTheatres: 6,
    showtimes: 7,
    noData: 8,
    qrCode: 9
  };

  var pebbleMessagesOut = {
    init: 0,
    getMovies: 1,
    getTheatres: 2,
    getTheatreMovies: 3,
    getMovieTheatres: 4,
    getShowtimes: 5,
    getQrCode: 6,
    pushPin: 7
  };

  var numberEncode = function (num) {
    return Number(num).toString(NUMBER_RADIX);
  };

  var numberDecode = function (str) {
    if (str.indexOf('$') > -1) {
      return parseInt(str.replace('$', ''), 10);
    }

    return parseInt(str, NUMBER_RADIX);
  };



  /**
   * 
   * @type PBMovies.service
   */
  var movieService;


  /**
   * 
   * @param {string} url
   * @param {type} method
   * @param {type} data
   * @param {type} successHandler
   * @param {type} errorHandler
   * @returns {undefined}
   */
  var makeRequest = function (url, method, data, successHandler, errorHandler) {
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

    xhr.onreadystatechange = function (e) {
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
              try {
                response = JSON.parse(xhr.responseText);
              } catch (e) {
                console.log("Malformed response: " + e);
                return errorHandler(xhr);
              }
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
  /**
   * 
   * @param {function} initDoneCallback description
   * @returns {PBMovies.service}
   */
  var PBMovies = function (initDoneCallback) {
    var locationInfo = {}, secretKey, deviceId;
    var locationOptions = {"timeout": LOCATION_TIMEOUT, "maximumAge": LOCATION_EXPIRY};
    var currentDate = dateYmd();
    var dateOffset = 0;
    var PostMethods = ["register", "pin-push"];
    var lastPbMsgIn;
    var isPreloading = false;
    //called on successful location detection
    var locationSuccess = function (pos) {
      var coordinates = pos.coords;
      locationInfo.latlng = coordinates.latitude + "," + coordinates.longitude;
      //fetchWeather(coordinates.latitude, coordinates.longitude);
      locationSet();
    };

    var isset = function (str) {
      return str && (str.length);
    };
//called on error
    var locationError = function (err) {
      console.warn('location error (' + err.code + '): ' + err.message);
      if (isset(locationInfo.postalCode) && (isset(locationInfo.city) || isset(locationInfo.postalCode))) {
        locationSet();
      } else {
        pebble.sendAppMessage({
          "code": pebbleMessagesIn.initFailed,
          "message": "Location Unavailable, check settings"
        });
      }
    };

    var locationSet = function () {
      console.log("Location is set" + JSON.stringify(locationInfo));
    };


    var _initRegistration = function () {
      secretKey = service.get(KEY_SECRET_KEY);
      deviceId = service.get(KEY_DEVICE_ID);
      if (!secretKey || !deviceId) {
        register();
      } else {
        registerDone();
      }
    };

    var register = function () {
      var deviceUUID = pebble.getAccountToken();
      console.log("UUID: " + deviceUUID);
      //console.log("Registering device...");
      service.proxy('register', {'device_uuid': deviceUUID}, function (resp) {
        service.store(KEY_SECRET_KEY, secretKey = resp.device.secret_key);
        service.store(KEY_DEVICE_ID, deviceId = resp.device.id);
        registerDone();
      }, function (xhr) {
        console.log("Response: " + xhr.responseText);
        pebble.sendAppMessage({
          "code": 0
              //"message": "Connection error"
        });
      });
    };

    var registerDone = function () {
      console.log("Registered: " + deviceId);
      initDoneCallback();
      window.setTimeout(preload, 700);
    };

    var preloadRetries = 0;
    var preload = function (loadCb, errorCb) {
      var cachedData = service.isCached(KEY_PRELOAD);
      var preloadFailed = function (xhr) {
        isPreloading = false;
        if (++preloadRetries > 4) {
          preloadRetries = 0;
          if (errorCb) {
            errorCb();
          }
        } else {
          window.setTimeout(function () {
            preload(loadCb, errorCb);
          }, PRELOAD_WAIT_TIME);
        }
        console.log(xhr.responseText);
      };

      //is caching or precached
      if (!cachedData && !isPreloading) {
        isPreloading = true;
        service.proxy('preload11', {'tries': preloadRetries},
            function (response) {
              if (response.data && !(response.data instanceof Array)) {
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
                  pebble.showSimpleNotificationOnPebble("Update Available", "A new version Pebble Movies has been published, visit Pebble App store to update!");
                  service.store("lastUpdateAlert", {'version': response.version, 'time': currentTimeInMs()}, true);
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

    var findIdsInList = function (list, ids, idIdx) {
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

    var findIdInList = function (list, id, idIdx) {
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

    var MessageHandler = {
      init: function () {
        initFunction();
      },
      getMovies: function () {
        MessageHandler.checkPreloaded(function (data) {
          var movies = data.movies;
          if (movies && movies.length > 0) {
            MessageHandler.sendData(pebbleMessagesIn.movies, movieUtils.convertToData(movies), 1, 0, 0);
          } else {
            MessageHandler.sendNoData("No movies now");
          }
        });
      },
      getMovieTheatres: function (dataIn) {
        MessageHandler.checkPreloaded(function (data) {
          var movieId = numberDecode(dataIn.movieId);
          var movie = findIdInList(data.movies, movieId);
          //theatres is index 7
          var theatres = movie ? findIdsInList(data.theatres, movie[7]) : [];
          if (theatres.length > 0) {
            MessageHandler.sendData(pebbleMessagesIn.movieTheatres, theatreUtils.convertToData(theatres));
          } else {
            MessageHandler.sendNoData("No theatres now");
          }
        });
      },
      getTheatres: function () {
        MessageHandler.checkPreloaded(function (data) {
          var theatres = data.theatres;
          if (theatres && theatres.length > 0) {
            MessageHandler.sendData(pebbleMessagesIn.theatres, theatreUtils.convertToData(theatres));
          } else {
            MessageHandler.sendNoData("No theatres now");
          }
        });
      },
      getTheatreMovies: function (dataIn) {
        MessageHandler.checkPreloaded(function (data) {
          var theatreId = numberDecode(dataIn.theatreId);
          var theatre = findIdInList(data.theatres, theatreId);
          //console.log("T: "+JSON.stringify(theatre)+ " Movies: "+ JSON.stringify(theatre[4]));
          var movies = theatre ? findIdsInList(data.movies, theatre[4], 0) : [];
          //console.log("TMovies: "+JSON.stringify(movies));
          if (movies.length > 0) {
            MessageHandler.sendData(pebbleMessagesIn.theatreMovies, movieUtils.convertToData(movies));
          } else {
            MessageHandler.sendNoData("No movies now");
          }
        });
      },
      checkPreloaded: function (callback) {

        var data = service.isCached(KEY_PRELOAD);
        if (isPreloading) {//if preloading, then wait
          window.setTimeout(function () {
            MessageHandler.checkPreloaded(callback);
          }, PRELOAD_WAIT_TIME);
        } else if (!data) {//not preloading, and no data, try to preload
          preload(function () {
            MessageHandler.checkPreloaded(callback);
          }, MessageHandler.handleErrors);
        } else {
          callback(data); //data preloaded, call callback
        }
      },
      getShowtimes: function (dataIn) {
        MessageHandler.checkPreloaded(function (data) {
          var key = numberDecode(dataIn.theatreId) + "." + numberDecode(dataIn.movieId);
          //console.log("Showtime Key: " + key);
          var showtimes = data.showtimes ? data.showtimes[key] : [];
          showtimeUtils.processShowtimes(showtimes || []);
        });
      },
      getQrCode: function (dataIn) {
        var showtimeId = numberDecode(dataIn.showtimeId);
        console.log('Making QR Code call for showtime: ' + showtimeId + " original: " + dataIn.showtimeId);
        var resourceUrl = service.proxy('qr', {"showtime_id": showtimeId}, null, MessageHandler.handleErrors, true);
        console.log("Resource URL: " + resourceUrl);
        downloadBinaryResource(resourceUrl, function (bytes) {
          transferBytes(bytes, MAX_DATA_LENGTH, pebbleMessagesIn.qrCode,
              function () {
                console.log("Done sending image!");
                //transferInProgress = false;
              },
              function (e) {
                console.log('Sending failed: ' + JSON.stringify(e));
              }
          );
        }, function (e) {
          console.log('QRCode could not downloaded');
        });
      },
      pushPin: function (dataIn) {
        var showtimeId = numberDecode(dataIn.showtimeId);
        console.log('Pushing Pin for Showtime : ' + showtimeId + " original: " + dataIn.showtimeId);
        pebble.showSimpleNotificationOnPebble("Reminder Set", "Timeline request is being processed.");
        var reminder = service.get(SETTING_REMINDER, false, DEFAULT_REMINDER);
        pebble.getTimelineToken(
            function (token) {
              service.proxy('pin-push', {
                showtimeId: showtimeId,
                reminder: reminder,
                timelineToken: token
              }, function (pin) {
                console.log("Pin was successfully generated, ID: " + pin.id);
              }, function (error) {
                pebble.showSimpleNotificationOnPebble('Reminder Failure', "The movie showtime could not be put in your timeline at the moment.");
              });
            },
            function (error) {
              console.log('Error getting timeline token: ' + error);
            }
        );
      },
      handleErrors: function (err) {
        //console.log("Errors occured while getting data :" + JSON.stringify(err));
        pebble.sendAppMessage({
          "code": pebbleMessagesIn.connectionError,
          "message": "Connection Error"
        });
      },
      truncateData: function (data) {
        var maxDataLength = MAX_DATA_LENGTH * MAX_PAGES;
        while (byteSize(data) > maxDataLength) {
          var tmp = data.substring(0, maxDataLength);
          data = tmp.substring(0, tmp.lastIndexOf(DELIMETER_RECORD));
        }
        return data;
      },
      sendData: function (msgCode, data, page, retries, successCallback) {
        lastPbMsgIn = msgCode;
        retries = retries || 0;
        page = page || 1;
        data = MessageHandler.truncateData(data);

        var totalPages = Math.ceil(data.length / MAX_DATA_LENGTH);
        var offset = (page - 1) * MAX_DATA_LENGTH;

        var outData = {
          "code": msgCode,
          "page": page,
          "data": data.substring(offset, MAX_DATA_LENGTH * page),
          "size": byteSize(data)
        };

        console.log("Sending page : " + page + " with " + byteSize(outData.data) + " bytes of " + byteSize(data));
        pebble.sendAppMessage(outData, function (e) {
          retries = 0;
          //Advance to next  page.
          if (page < totalPages && page < MAX_PAGES) {
            MessageHandler.sendData(msgCode, data, page + 1, retries, successCallback);
          } else {
            if (successCallback) {
              successCallback();
            }
          }

        }, function (e) {
          if (retries++ < 3) {
            //retry same page
            console.log("Retrying... [" + retries + "] previous failed: " + JSON.stringify(e));
            MessageHandler.sendData(msgCode, data, page, retries, successCallback);
          } else {
            console.log("Failed...");
          }
        });

      },
      sendNoData: function (msg) {
        pebble.sendAppMessage({
          "code": pebbleMessagesIn.noData,
          "message": msg
        });
      }
    };

    var clean = function (record) {
      for (var i in record) {
        if (typeof record[i] === "string" && (record[i].indexOf(DELIMETER_FIELD) > -1 || record[i].indexOf(DELIMETER_RECORD) > -1)) {
          record[i] = record[i].replace(DELIMETER_FIELD, "/").replace(DELIMETER_RECORD, " ").trim();
        }

      }
      return record;
    };

    var showtimeUtils = {
      //"id", "time", "type", "link"
      processShowtimes: function (showtimes) {
        var records = [];
        for (var i = 0; i < showtimes.length; i++) {
          var showtime = [];
          var time = showtimeUtils.formatTime(currentDate, showtimes[i][1]);
          showtime.push(numberEncode(showtimes[i][0]));
          showtime.push(showtimeTypeMask[showtimes[i][2]]);
          showtime.push(time);
          showtime.push(showtimes[i][3] ? 1 : 0);
          records.push(showtime.join(DELIMETER_FIELD));
        }
        //bug 
        if (records.length) {
          var tlFeatureNotify = function () {
            var isNotified = service.get(FEATURE_NOTIFY_TIMELINE, false, false);
            //show alert ?
            if (!isNotified) {
              pebble.showSimpleNotificationOnPebble("Timeline Feature", "New feature: You can now add  movie tmes to your timeline. To add a movie time's pin, click the select button twice with the desired movie time selected.\n\nYou can still get QR Code by simply tapping the select button.\n\nTo set the time for reminders check the settings page.");
              service.store(FEATURE_NOTIFY_TIMELINE, true, false);
            }
          };
          //console.log("Showtimes: " + JSON.stringify(records));
          MessageHandler.sendData(pebbleMessagesIn.showtimes, records.join(DELIMETER_RECORD), 1, 0, tlFeatureNotify);
        } else {
          MessageHandler.sendNoData("No showtimes");
        }
      },
      formatTime: function (showdate, showtime) {
        var arr = (showdate + " " + showtime).split(/[- :T]/);
//And use the constructor with 6 args
        var d = new Date(arr[0], arr[1] - 1, arr[2], arr[3], arr[4], 0);
        //var d = new Date(showdate + " " + showtime);
        var min = d.getMinutes();
        var hours = d.getHours();
        var hoursHp = hours % 12 || 12;
        return (hoursHp > 9 ? hoursHp : "0" + hoursHp) + ":" +
            (min > 9 ? min : "0" + min) +
            (hours >= 12 ? "PM" : "AM");

      }
    };
    var theatreUtils = {
      formatDistance: function (dist) {
        var distNum = parseFloat(dist);
        if (distNum < 1 || isNaN(distNum)) {
          return " ";
        }
        var prefUnit = service.get(SETTING_DEFAULT_UNIT, false, DISTANCE_UNIT_KM);
        var conversion = prefUnit === DISTANCE_UNIT_KM ? DISTANCE_KM_IN_M : DISTANCE_MILE_IN_M;
        return Number(conversion * distNum).toFixed(1) + prefUnit;
      },
      convertToData: function (theatres) {
        var theatre, records = [];
        sort_in_place(theatres, 3, function (t1, t2) {
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
          theatre[0] = numberEncode(theatre[0]);
          theatre[3] = theatreUtils.formatDistance(theatre[3]);
          theatre = theatre.slice(0, 4);
          theatre = clean(theatre);
          records.push(theatre.join(DELIMETER_FIELD));
        }
        return records.join(DELIMETER_RECORD);
      }
    };

    var movieUtils = {
      convertToData: function (movies) {
        sort_in_place(movies, 'title');
        var movie, records = [];
        for (var i = 0; i < movies.length; i++) {
          //id,title,genre,user_rating,rated,critic_rating,runtime                    
          try {
            movie = movies[i];
            movie[0] = numberEncode(movie[0]);
            movie[2] = movie[2] || " ";
            movie[3] = movie[3] ? Number(movie[3] * 5).toPrecision(2) + "/5" : '0';
            movie[4] = movie[4] ? movie[4] + "" : "NR";
            movie[4] = !movie[4].trim().length ? "NR" : movie[4];
            movie[4] = movie[4].replace(/(Not Rated)|(Unrated)/i, "NR");
            movie[5] = movie[5] ? Math.min(Math.round(parseFloat(movie[5]) * 100), 99) : '0';
            movie = clean(movie);
            movie = movie.slice(0, 7);
          } catch (e) {
            console.log("Error " + movie[4] + "__" + e.message);
          }
          records.push(movie.join(DELIMETER_FIELD));
        }

        return records.join(DELIMETER_RECORD);
      }
    };

    var service = {
      unStore: function (key) {
        if (window.localStorage) {
          delete window.localStorage[key];
        }
      },
      isStored: function (key) {
        return window.localStorage && window.localStorage[key] !== undefined;
      },
      store: function (key, val, asObject) {
        try {
          window.localStorage.setItem(key, asObject ? JSON.stringify(val) : val);
        } catch (e) {
          console.log("Error occured while saving item");
        }
      },
      get: function (key, asObject, defaultValue) {
        try {
          if (window.localStorage && window.localStorage[key]) {
            return asObject ? JSON.parse(window.localStorage.getItem(key)) : window.localStorage.getItem(key);
          }
        } catch (e) {
          console.log("Storage Object error");
        }
        return defaultValue !== undefined ? defaultValue : undefined;
      },
      proxy: function (command, data, successCallback, errorCallback, urlOnly) {
        var method = PostMethods.indexOf(command) > -1 ? "POST" : "GET";
        var urlData = {token: service.signRequest(), 'date': currentDate, 'version': CURRENT_VERSION, 'dateOffset': dateOffset};
        var forceLocation = parseInt(service.get(SETTING_FORCE_LOCATION, false, "0"));

        for (var i in locationInfo) {
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
        console.log("URL: " + url);
        return makeRequest(url, method, reqData, successCallback, errorCallback);
      },
      signRequest: function () {
        //token requestId|deviceId|sign=sha1(requestId.deviceId.secretKey)
        var requestId = currentTimeInMs();
        return [requestId, deviceId, sha1(requestId + deviceId + secretKey)].join("|");
      },
      postRequest: function (url, data, successHandler, errorHandler) {
        return makeRequest(url, data, 'POST', successHandler, errorHandler);
      },
      getRequest: function (url, successHandler, errorHandler) {
        return makeRequest(url, 'GET', successHandler, errorHandler);
      },
      cacheKey: function (dataKey, offset) {
        return 'cache_' + dataKey + "_" + dateYmdWithOffset(offset !== undefined ? offset : dateOffset);
      },
      isCached: function (dataKey, ignoreExpiry) {
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
      cache: function (dataKey, data, validity) {
        var k = service.cacheKey(dataKey);
        validity = validity ? validity : CACHE_EXPIRY;
        return service.store(k, {expiry: currentTimeInMs() + validity, 'data': data}, true);
      },
      uncache: function (dataKey, offset) {
        var k = service.cacheKey(dataKey, offset);
        service.unStore(k);
      },
      serviceError: function (cacheKey, xhr, onSuccess, onError) {
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
      handleMessage: function (payload) {
        var msgCode = parseInt(payload.code);

        if (payload.dateOffset !== undefined) {
          dateOffset = payload.dateOffset;
        } else {
          dateOffset = 0;
        }
        //console.log("Handling message... "+JSON.stringify(payload));
        for (var i in pebbleMessagesOut) {
          if (pebbleMessagesOut[i] === msgCode) {
            if (MessageHandler.hasOwnProperty(i)) {
              //console.log("Handler found ");
              return MessageHandler[i](payload);
            } else {
              console.log("Message handler does not have method for: " + i);
              return null;
            }
          }
        }
        console.log("can't handle message code: " + msgCode);
      },
      loadLoactionInfo: function () {
        locationInfo.postalCode = service.get(SETTING_DEFAULT_POSTAL_CODE, false, "");
        locationInfo.city = service.get(SETTING_DEFAULT_CITY, false, "");
        locationInfo.country = service.get(SETTING_DEFAULT_COUNTRY, false, "");
      }

    };

    var init = function () {
      //var locationInfo = service.get('location', true);
      service.loadLoactionInfo();

      pebble.getTimelineToken(
          function (token) {
            console.log('Application Launcing, timelne token is: ' + token);
          },
          function (error) {
            console.log('Error getting timeline token: ' + error);
          }
      );

      window.navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions);
      _initRegistration();
    };


    init();
    return service;
  };

  var initFunction = function (sends) {
    sends = sends || 1;
    movieService = movieService || new PBMovies(function () {
      window.setTimeout(function () {
        pebble.sendAppMessage({
          "code": pebbleMessagesIn.startApp
        }, function (e) {
          console.log("App started!");
        }, function (e) {
          console.log("Init failed: ");
          if (sends++ < 4) {
            initFunction(sends);
          }
        });
      }, MIN_SPLASH_TIME);
    });
    //movieService.unStore("secretKey");
  };
//the pebble app itself

  pebble.addEventListener("ready", function (e) {
    initFunction();
  });


  pebble.addEventListener("appmessage",
      function (e) {
        //console.log("Received message: " + JSON.stringify(e.payload));
        if (movieService) {
          movieService.handleMessage(e.payload);
        }
      }
  );


  pebble.addEventListener("webviewclosed", function (e) {
    //console.log("configuration closed");
    // webview closed
    var options = JSON.parse(decodeURIComponent(e.response));
    var keys = [SETTING_DEFAULT_CITY, SETTING_DEFAULT_COUNTRY, SETTING_DEFAULT_POSTAL_CODE, SETTING_DEFAULT_UNIT, SETTING_FORCE_LOCATION, SETTING_REMINDER];
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
      for (var j = 0; j < DAYS_TO_BROWSE; j++) {
        movieService.uncache(KEY_PRELOAD, j);
      }
      movieService.loadLoactionInfo();
    }

    console.log("Options = " + JSON.stringify(options));
  });


  pebble.addEventListener("showConfiguration", function () {
    console.log("showing configuration");
    var proxyUrl = movieService.proxy('settings', {
      'unit': movieService.get(SETTING_DEFAULT_UNIT, false, DISTANCE_UNIT_KM),
      'forceLocation': movieService.get(SETTING_FORCE_LOCATION, false, "0"),
      'Reminder': movieService.get(SETTING_REMINDER, false, DEFAULT_REMINDER)
    }, null, null, true);
    console.log("opening settings url: " + proxyUrl);
    pebble.openURL(proxyUrl);
  });


  function serializeData(data) {
    if (data) {
      if (typeof data !== "object") {
        return data;
      }

      var parts = [];
      for (var i in data) {
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

  function transferBytes(bytes, chunkSize, msgCode, successCb, failureCb) {
    var retries = 0;

    var success = function () {
      if (successCb !== undefined) {
        successCb();
      }
    };

    var failure = function (e) {
      if (failureCb !== undefined) {
        failureCb(e);
      }
    };

    // This function sends chunks of data.
    var sendChunk = function (start) {
      var ending = start + chunkSize;
      console.log('Sending ' + start + ' to ' + ending + ' of ' + bytes.length + 'bytes');
      var page = Math.floor(start / chunkSize) + 1;
      pebble.sendAppMessage({
        "data": bytes.slice(start, ending),
        "code": msgCode,
        "page": page,
        "size": bytes.length
      }, function (e) {
        // If there is more data to send - send it.
        if (bytes.length > (start + chunkSize)) {
          sendChunk(start + chunkSize);
        } else {
          success();
        }
      }, function (e) {//retry
        if (retries++ < 3) {
          console.log("Got a nack for chunk #" + start + " - Retry...");
          sendChunk(start);
        } else {
          failure(e);
        }
      }
      );
    };

    sendChunk(0);
  }

  function downloadBinaryResource(imageURL, callback, errorCallback) {
    var req = new XMLHttpRequest();
    req.open("GET", imageURL, true);
    req.responseType = "arraybuffer";
    req.onreadystatechange = function (e) {
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
        } else {
          errorCallback("Request status is " + req.status);
        }
      }
    };
    req.onerror = function (e) {
      errorCallback(e);
    };
    req.send(null);
  }

  function stringToBytes(str) {
    var s = unescape(encodeURIComponent(str));
    var bufView = [];
    for (var i = 0, strLen = s.length; i < strLen; i++) {
      bufView.push(s.charCodeAt(i));
    }

    return bufView;
  }

  function byteSize(str) {
    return window.unescape(window.encodeURIComponent(str)).length;
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
    o = o || 0;
    return dateYmd(currentTimeInMs() + o * 86400000);
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
    var d = function (n, s) {
      var a = (n << s) | (n >>> (32 - s));
      return a;
    };
    var e = function (a) {
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
    c = utf8_encode(c);
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
        break;
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
})(Pebble);