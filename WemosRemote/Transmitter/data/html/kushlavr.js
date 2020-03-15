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

KVR.lastFD = {
    x: 0,
    y: 0,
    ch3: 0,
    ch4: 0
};

KVR.Autorefresh = function () {

    var changed = false;
    var fd = {
    };

    var x = $('[name=x]')[0].value;
    if (x !== KVR.lastFD.x) {
        fd.x = x;
        KVR.lastFD.x = x;
        changed = true;
    }

    var y = $('[name=y]')[0].value;
    if (y !== KVR.lastFD.y) {
        fd.y = y;
        KVR.lastFD.y = y;
        changed = true;
    }

    var ch3 = $('[name=ch3]')[0].value;
    if (ch3 !== KVR.lastFD.ch3) {
        fd.ch3 = ch3;
        KVR.lastFD.ch3 = ch3;
        changed = true;
    }

    var ch4 = $('[name=ch4]')[0].value;
    if (ch4 !== KVR.lastFD.ch4) {
        fd.ch4 = ch4;
        KVR.lastFD.ch4 = ch4;
        changed = true;
    }
    if (changed === true) {

        var jqxhr = $.ajax({
            type: "POST",
            url: "/api/setup",
            data: fd
        })
            .done(function (data) {
                $.each(data, function (name, value) {
                    console.log("Name: " + name + ", Value: " + value);
                    var form = $("#form");
                    $("[name=" + name + "]", form).val(value);
                });
                setTimeout(function () {
                    KVR.Autorefresh();
                }, 200);
            })
            .fail(function () {
                console.log("error");
                setTimeout(function () {
                    KVR.Autorefresh();
                }, 200);
            });
    } else {
        setTimeout(function () {
            KVR.Autorefresh();
        }, 200);
    }


};

KVR.setValue = function (inputName, value) {
    $("[name=" + inputName + "]").val(value);
}