var KVR = {};

KVR.load = function () {
    $.get("/api/setup")
        .done(function (data) {
            $.each(data, function (name, value) {
                KVR.setValue(name, value);
            });
        })
        .fail(function () {
            console.log("error");
        });
};

KVR.Autorefresh = function () {
    $.get("/api/values")
        .done(function (data) {
            $.each(data, function (name, value) {
                KVR.setValue(name, value);
            });
            setTimeout(function () {
                KVR.Autorefresh();
            }, 1000);
        })
        .fail(function () {
            console.log("error");
            setTimeout(function () {
                KVR.Autorefresh();
            }, 1000);
        });
};

KVR.setValue = function (inputName, value) {
    console.log("Name: " + inputName + ", Value: " + value);
    var form = $("#form");
    var input = $("[name=" + inputName + "]", form);
    if (input.length > 0) {
        if (input[0].tagName === "INPUT") {
            input.val(value);
        } else {
            input.html(value);
        }
    }
};