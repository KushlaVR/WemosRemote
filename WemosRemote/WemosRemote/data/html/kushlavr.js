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
