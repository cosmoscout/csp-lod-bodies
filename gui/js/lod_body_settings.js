init_slider("set_terrain_height", 1.0, 20.0, 0.1, [1]);
init_slider("set_height_range", -12.0, 21.0, 0.1, [-8, 12]);
init_slider("set_slope_range", 0.0, 90.0, 1.0, [0, 45]);
init_slider("set_terrain_lod", 10.0, 50.0, 0.1, [15]);
init_slider("set_texture_gamma", 0.1, 3.0, 0.01, [1.0]);


$('#set_enable_auto_terrain_lod').change(function () {
    if (this.checked)
        $("#set_terrain_lod").addClass("unresponsive");
    else
        $("#set_terrain_lod").removeClass("unresponsive");
});


function set_map_data_copyright(text) {
    $("#img-data-copyright").tooltip({title: "© " + text, placement: "top"});
}

function set_elevation_data_copyright(text) {
    $("#dem-data-copyright").tooltip({title: "© " + text, placement: "bottom"});
}