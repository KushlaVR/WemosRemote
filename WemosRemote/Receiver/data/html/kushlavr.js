var KVR = {};

KVR.load = function () {
    var jqxhr = $.get("/api/setup")
        .done(function (data) {
            $.each(data, function (name, value) {
                console.log("Name: " + name + ", Value: " + value);
                var form = $("#form");
                $("[name=" + name + "]", form).val(value);
            });
        })
        .fail(function () {
            console.log("error");
        });
};

KVR.Autorefresh = function () {
    var jqxhr = $.get("/api/values")
        .done(function (data) {
            $.each(data, function (name, value) {
                console.log("Name: " + name + ", Value: " + value);
                var form = $("#form");
                $("[name=" + name + "]", form).val(value);
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
    $("[name=" + inputName + "]").val(value);
}