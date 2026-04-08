var GlobalLayerIndexes = 0;

$.fn.isBound = function(type, fn) {
    var data = this.data('events')[type];

    if (data === undefined || data.length === 0) {
        return false;
    }

    return (-1 !== $.inArray(fn, data));
};

function AtlasLayer(id, on, name, type, textureName, scale = {x: 1.0, y: 1.0}, offset = {x: 0.0, y: 0.0}) {
    this.on = on;
    this.index = GlobalLayerIndexes++;
    this.name = name;
    this.type = type;
    this.id = id;
    this.textureName = textureName;
    this.scale = scale;
    this.offset = offset;
    this.tiles = [];
}

AtlasLayer.prototype.addTile = function(tile) {
    this.tiles.push(tile);
}

AtlasLayer.prototype.reset = function(tile) {
    this.tiles = [];
}

function Settings() {
    this.props = new Map();
}

Settings.prototype.get = function(key, def = null) {
    return this.props.has(key) ? this.props.get(key) : def;
}

Settings.prototype.set = function(key, value) {
    this.props.set(key, value);
}

Settings.prototype.assign = function(obj) {
    this.props = new Map(Object.entries(obj));
}

Settings.prototype.objToStrMap = function(obj) {

    let strMap = new Map();

    for (let k of Object.keys(obj)) {
        strMap.set(k, obj[k]);
    }
    return strMap;
}

Settings.prototype.toJSON = function() {
    let jsonObject = {};
    for (let [key, value] of this.props.entries()) {
        jsonObject[key] = value;
    }
    this.props.forEach((value, key) => {
    });
    return JSON.stringify(jsonObject);
}

Settings.prototype.fromJSON = function(jsonStr) {

    var result = null;

    try {
        result = this.objToStrMap(JSON.parse(jsonStr));
        this.props = new Map(result);
        console.log(this.props);
        resetValues();
        init();
    } catch(e) {
        alert(e);
    }

}

var sprintf = (str, ...argv) => !argv.length ? str : sprintf(str = str.replace(sprintf.token||"$", argv.shift()), ...argv);

const WALL_FRONT = 0;
const WALL_SIDE = WALL_FRONT+1;

const LAYER_TYPE_WALLS = 0;
const LAYER_TYPE_FLOOR = LAYER_TYPE_WALLS+1;
const LAYER_TYPE_CEILING = LAYER_TYPE_FLOOR+1;
const LAYER_TYPE_DECAL = LAYER_TYPE_CEILING+1;
const LAYER_TYPE_OBJECT = LAYER_TYPE_DECAL+1;

let settings = new Settings();

let transparentBackgroundColor = {r: 255, g: 0, b: 255};

let renderer = null;
let glcanvas = null;
let imageCanvas = null;
let camera = null;
let scene = null;
let light = null;
let previewObjectsGroup = null;
let generateObjectGroup = null;

let wallObjectsGroup = null;
let decalObjectsGroup = null;
let floorObjectsGroup = null;
let ceilingObjectsGroup = null;
let objectObjectsGroup = null;
let canvasContainer;
let loadingManager;
let bufferTexture = null;
let whiteTexture = null;

let vertexShader;
let fragmentShader;

let canvas;
let ctx;
let ctx2D;

let squaresToGenerateList = [];
let packedCanvas;
let atlas;
let totalTilesToPack = 0;
let currentlyPackedTilesNumber = 0;
let loadedImages = [];

let textureLoader;
let isLoadingTexture = false;

let textureFilenames = [
    {name: "eob_bricks.png", filename: "/tools/atlas_generator/textures/grey_bricks.png"},
    {name: "eob_keyhole.png", filename: "/tools/atlas_generator/textures/keyhole.png"}
];

let layerTypes = new Map;
layerTypes.set(LAYER_TYPE_WALLS, "Walls");
layerTypes.set(LAYER_TYPE_FLOOR, "Floor");
layerTypes.set(LAYER_TYPE_CEILING, "Ceiling");
layerTypes.set(LAYER_TYPE_DECAL, "Decal");
layerTypes.set(LAYER_TYPE_OBJECT, "Object");

let generatedData = {
    version: null,
    generated: null,
    resolution: {
        width: 0,
        height: 0
    },
    depth: 0,
    width: 0,
    layers: [
        new AtlasLayer(1, true, "Grey stone floor", LAYER_TYPE_FLOOR, "eob_bricks.png"),
        new AtlasLayer(2, true, "Grey stone ceiling", LAYER_TYPE_CEILING, "eob_bricks.png"),
        new AtlasLayer(3, true, "Grey stone walls", LAYER_TYPE_WALLS, "eob_bricks.png"),
        new AtlasLayer(4, true, "Keyhole object", LAYER_TYPE_OBJECT, "eob_keyhole.png", {x: 0.25, y: 0.5}, {x: 0.0, y: -0.15})
    ]
}

let textures = [];

function mainloop() {
    render();
    requestAnimationFrame(mainloop);
}

function clamp(num, min, max) {
  return Math.min(Math.max(num, min), max);
};

function initUI() {

    $(".slider").each(function() {
        $(this).prev('input').attr("id", $(this).attr("id"));
        $(this).prev('input').val(settings.get($(this).attr("id")));
        $(this).slider({
            value: settings.get($(this).attr("id")),
            min: parseFloat($(this).attr("minValue")),
            max: parseFloat($(this).attr("maxValue")),
            step: parseFloat($(this).attr("step")),
            create: function( event, ui ) {
                var slider = $(this);
                $(this).prev('input').change(function() {
                    var v = parseFloat($(this).val());
                    if (v || v == 0) {
                        v = clamp(v, $(slider).attr("minValue"), slider.attr("maxValue"));
                        $(this).val(v);
                        slider.slider("option", "value", v);
                        settings.set($(this).attr("id"), v);
                        init();
                    }
                });
            },
            slide: function(event, ui) {
                $(this).prev('input').val(ui.value);
                settings.set($(this).attr("id"), ui.value);
                init();
            }
        });

    })

    var wireColor = settings.get("wireColor");
    var defaultColor = 'rgb('+wireColor.r+','+wireColor.g+','+wireColor.b+')';

    $('.cp-alt-target1').colorpicker({
        color: defaultColor,
        inline:false,
        altField: '.cp-alt-target1',
        altProperties: 'background-color',
        altAlpha: false,
        alpha: false,
        ok: function(event, color) {
            settings.set("wireColor", {r: Math.round(255*color.rgb.r), g: Math.round(255*color.rgb.g), b: Math.round(255*color.rgb.b)});
            init();
        }
    });

    var fogColor = settings.get("fogColor");
    var defaultColor = 'rgb('+fogColor.r+','+fogColor.g+','+fogColor.b+')';

    $('.cp-alt-target2').colorpicker({
        color: defaultColor,
        inline:false,
        altField: '.cp-alt-target2',
        altProperties: 'background-color',
        altAlpha: false,
        alpha: false,
        ok: function(event, color) {
            settings.set("fogColor", {r: Math.round(255*color.rgb.r), g: Math.round(255*color.rgb.g), b: Math.round(255*color.rgb.b)});
        }
    });

    $('#selectBackgroundImage').change(function() {
        init();
    });

    $('#checkboxUseShading').change(function() {
        settings.set("useFog", this.checked);
    });

    $('#checkboxTextureFiltering').change(function() {
        settings.set("textureFiltering", this.checked);
    });

    var row = '<div class="header">';
    row += sprintf('<div class="cell" style="width:30px;">$</div>', "On");
    row += sprintf('<div class="cell" style="width:150px;">$</div>', "Type");
    row += sprintf('<div class="cell" style="width:100px;">$</div>', "ID");
    row += sprintf('<div class="cell" style="width:200px;">$</div>', "Name");
    row += sprintf('<div class="cell" style="width:150px;">$</div>', "Texture");
    row += '</div>';
    $("#layersContainer").append(row);

    generatedData.layers.forEach((layer, i) => {
        $("#layersContainer").append(getLayerHTML(layer));
    });

    updateLayerEvents();
}

function prepareGeneratedJSON() {

    generatedData.resolution = {
        width: settings.get("viewportWidth"),
        height: settings.get("viewportHeight")
    }
    generatedData.depth = settings.get("dungeonDepth");
    generatedData.width = settings.get("dungeonWidth");

    var data = JSON.parse(JSON.stringify(generatedData));
    var excludeLayers = [];
    data.layers.forEach((item, i) => {
        delete item.textureName;
        item.type = layerTypes.get(item.type);
        if (!item.on) {
            excludeLayers.push(item.id);
        }
    });

    excludeLayers.forEach((layerId, i) => {
        data.layers = deleteExcludedLayer(data.layers, layerId);
    });

    var jsonString = JSON.stringify(data, undefined, 2);

    return JSON.minify(jsonString);

}

function loadPreset() {

    $('#loadPresetFile').unbind();

    $('#loadPresetFile').on("change", function(evt) {
        var target = evt.target || evt.srcElement;
        if (target.files.length > 0) {
            var file = evt.target.files[0];

            var reader = new FileReader();
            reader.onload = function(e) {
                settings.fromJSON(reader.result);
                document.getElementById("loadPresetForm").reset();
            };
            reader.readAsText(file);
        }

    });
    $('#loadPresetFile').click();

}

function savePreset() {

    var blob = new Blob([settings.toJSON()], {
        "type": "application/json"
    });
    var a = document.createElement("a");
    a.download = 'DCAG_settings_preset.json';
    a.href = URL.createObjectURL(blob);
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);

}

function saveJSON() {

    var blob = new Blob([prepareGeneratedJSON()], {
        "type": "application/json"
    });
    var a = document.createElement("a");
    a.download = 'atlas.json';
    a.href = URL.createObjectURL(blob);
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);

}

function updateLayerEvents() {

    $('#layersContainer button.deleteLayer').each(function() {
        $(this).unbind("click", deleteLayer);
        $(this).bind("click", deleteLayer);
    });

    $('#layersContainer button.moveLayerUp').each(function() {
        $(this).unbind("click", moveLayerUp);
        $(this).bind("click", moveLayerUp);
    });

    $('#layersContainer button.moveLayerDown').each(function() {
        $(this).unbind("click", moveLayerDown);
        $(this).bind("click", moveLayerDown);
    });

    $('#layersContainer .layerType').each(function() {
        $(this).unbind("change", updateLayer);
        $(this).bind("change", updateLayer);
    });

    $('#layersContainer .textureType').each(function() {
        $(this).unbind("change", updateLayer);
        $(this).bind("change", updateLayer);
    });

    $('#layersContainer .layerToggle').each(function() {
        $(this).unbind("change", updateLayer);
        $(this).bind("change", updateLayer);
    });

    $('#layersContainer .decalScaleX').each(function() {
        $(this).unbind("change", updateLayer);
        $(this).bind("change", updateLayer);
    });

    $('#layersContainer .decalScaleY').each(function() {
        $(this).unbind("change", updateLayer);
        $(this).bind("change", updateLayer);
    });

    $('#layersContainer .decalOffsetY').each(function() {
        $(this).unbind("change", updateLayer);
        $(this).bind("change", updateLayer);
    });

    $('#layersContainer .layerName').each(function() {
        $(this).unbind("change", updateLayer);
        $(this).bind("change", updateLayer);
    });

    $('#layersContainer .layerId').each(function() {
        $(this).unbind("change", updateLayer);
        $(this).bind("change", updateLayer);
    });

}

function addLayer() {
    var layer = new AtlasLayer("", true, "Untitled", LAYER_TYPE_WALLS, -1);
    generatedData.layers.push(layer);
    $("#layersContainer").append(getLayerHTML(layer));
    updateLayerEvents();
}

function getLayerHTML(layer) {

    var row = sprintf('<div class="row" layerId="$">', layer.index);

    var checked = layer.on ? "checked" : "";
    row += sprintf('<div class="cell" style="width:30px;"><input type="checkbox" class="layerToggle" layerId="$" $/></div>', layer.index, checked);

    var selectType = sprintf('<select class="layerType" layerId="$">', layer.index);
    for (let [key, value] of layerTypes.entries()) {
        var selected = key == layer.type ? "selected" : "";
        selectType += sprintf('<option $ value="$">$</option>', selected, key, value);
    }
    selectType += '</select>';

    var textureType = sprintf('<select class="textureType" layerId="$">', layer.index);
    var selected = layer.textureName == "" ? "selected" : "";
    textureType += sprintf('<option $ value="$">$</option>', selected, -1, "-- NONE --");
    textures.forEach((texture, i) => {
        selected = texture.name == layer.textureName ? "selected" : "";
        textureType += sprintf('<option $ value="$">$</option>', selected, texture.name, texture.name);
    });
    textureType += '</select>';


    row += sprintf('<div class="cell" style="width:150px;">$</div>', selectType);
    row += sprintf('<div class="cell" style="width:100px;"><input type="text" class="layerId" value="$" layerId="$" size="5" maxlength="255"/></div>', layer.id, layer.index);
    row += sprintf('<div class="cell" style="width:200px;"><input type="text" class="layerName" value="$" layerId="$" size="20" maxlength="255"/></div>', layer.name, layer.index);
    row += sprintf('<div class="cell">$</div>', textureType);
    row += '<div class="cell" style="width:120px; position:absolute; left:870px; top:0px;">';
    row += sprintf('<button layerId="$" title="Move layer down" class="btn moveLayerDown"><i class="fa fa-arrow-down"></i></button>', layer.index);
    row += sprintf('<button layerId="$" title="Move layer up" class="btn moveLayerUp"><i class="fa fa-arrow-up"></i></button>', layer.index);
    row += sprintf('<button layerId="$" title="Delete layer" class="btn deleteLayer"><i class="fa fa-remove"></i></button>', layer.index);
    row += '</div>';

    var visibleDecalProperties = (layer.type == LAYER_TYPE_DECAL || layer.type == LAYER_TYPE_OBJECT) ? "block" : "none";
    row += sprintf('<div class="decalPropertiesContainer" style="display:$;">', visibleDecalProperties);
    row += sprintf('<span>');
    row += sprintf('Scale X: <input class="decalScaleX" layerId="$" type="text" value="$" size="3" maxlength="10"/>',  layer.index, layer.scale.x);
    row += sprintf('Scale Y: <input class="decalScaleY" layerId="$" type="text" value="$" size="3" maxlength="10"/>',  layer.index, layer.scale.y);
    row += sprintf('Offset Y: <input class="decalOffsetY" layerId="$" type="text" value="$" size="3" maxlength="10"/>',  layer.index, layer.offset.y);
    row += '</span>';
    row += '</div>';


    row += '</div>';

    return row;

}

function updateLayer() {

    var index = $(this).attr("layerId");
    var className = $(this).attr("class");

    for(var i=0;i<generatedData.layers.length;i++) {
        if (generatedData.layers[i].index == index) {
            switch(className) {
                case "layerToggle": {
                    generatedData.layers[i].on = $(this).prop("checked");
                } break;
                case "layerType": {
                    generatedData.layers[i].type = parseInt($(this).val());
                    if (generatedData.layers[i].type == LAYER_TYPE_DECAL || generatedData.layers[i].type == LAYER_TYPE_OBJECT) {
                        $(this).parent().nextAll(".decalPropertiesContainer").show();
                    } else {
                        $(this).parent().nextAll(".decalPropertiesContainer").hide();
                    }
                } break;
                case "textureType": {
                    generatedData.layers[i].textureName = $(this).val();
                } break;
                case "layerId": {
                    var v = parseInt($(this).val());
                    if (v == 0 || v) {
                        $(this).val(v);
                        generatedData.layers[i].id = v;
                    } else {
                        $(this).val(generatedData.layers[i].id);
                    }
                } break;
                case "layerName": {
                    generatedData.layers[i].name = $(this).val();
                } break;
                case "decalScaleX": {
                    var v = parseFloat($(this).val());
                    if (v == 0 || v) {
                        v = clamp(v, 0.001, 1);
                        $(this).val(v);
                        generatedData.layers[i].scale.x = v;
                    } else {
                        $(this).val(generatedData.layers[i].scale.x);
                    }
                    init();
                } break;
                case "decalScaleY": {
                    var v = parseFloat($(this).val());
                    if (v == 0 || v) {
                        v = clamp(v, 0.001, 1);
                        $(this).val(v);
                        generatedData.layers[i].scale.y = v;
                    } else {
                        $(this).val(generatedData.layers[i].scale.y);
                    }
                    init();
                } break;
                case "decalOffsetY": {
                    var v = parseFloat($(this).val());
                    if (v == 0 || v) {
                        v = clamp(v, -1, 1);
                        $(this).val(v);
                        generatedData.layers[i].offset.y = v;
                    } else {
                        $(this).val(generatedData.layers[i].offset.y);
                    }
                    init();
                } break;
            }
            return;
        }
    }

}

function deleteLayer() {

    var index = $(this).attr("layerId");

    $(this).blur();

    for(var i=0;i<generatedData.layers.length;i++) {
        if (generatedData.layers[i].index == index) {
            generatedData.layers.splice(i, 1);
            $('#layersContainer .row[layerId='+index+']').remove();
            return;
        }
    }

}

function array_move(arr, old_index, new_index) {
    if (new_index >= arr.length) {
        var k = new_index - arr.length + 1;
        while (k--) {
            arr.push(undefined);
        }
    }
    arr.splice(new_index, 0, arr.splice(old_index, 1)[0]);
    return arr; // for testing
};


function moveLayerUp() {

    var index = $(this).attr("layerId");

    $(this).blur();

    for(var i=0;i<generatedData.layers.length;i++) {
        if (generatedData.layers[i].index == index) {
            if (i > 0) {
                generatedData.layers = array_move(generatedData.layers, i, i-1);
                var e = $('#layersContainer .row[layerId='+index+']');
                e.prev().insertAfter(e);
            }
            return;
        }
    }

}

function moveLayerDown() {

    var index = $(this).attr("layerId");

    $(this).blur();

    for(var i=0;i<generatedData.layers.length;i++) {
        if (generatedData.layers[i].index == index) {
            if (i < generatedData.layers.length-1) {
                generatedData.layers = array_move(generatedData.layers, i, i-1);
                var e = $('#layersContainer .row[layerId='+index+']');
                e.next().insertBefore(e);
            }
            return;
        }
    }

}

function copyJSONToClipboard() {

  var textarea = $("#jsonOutputContainer textarea");

  textarea.select();
  if (textarea.setSelectionRange) {
      textarea.setSelectionRange(0, 99999); /*For mobile devices*/
  }

  document.execCommand("copy");

  if (document.getSelection) {document.getSelection().removeAllRanges();}
  else if (document.selection) {document.selection.empty();}

  textarea.blur();

  $("#btnCopyToClipboard").prop('disabled', true);
  $("#clipboardCheckmark").css("position", "absolute");
  $("#clipboardCheckmark").css("left", "676px");
  $("#clipboardCheckmark").css("top", "8px");
  $("#clipboardCheckmark").fadeIn(100, function() {});
  $("#clipboardCheckmark").delay(2000).fadeOut(100, function() {
    $(this).hide();
    $("#btnCopyToClipboard").prop('disabled', false);
  });

}

function getRenderTextureData() {

  var w = bufferTexture.width;
  var h = bufferTexture.height;

  var rtData = new Uint8Array(w * h * 4);
  renderer.readRenderTargetPixels(bufferTexture, 0, 0, w, h, rtData);

  var data = new Uint8Array(w * h * 4);
  var texture = new THREE.DataTexture(data, w, h,THREE.RGBAFormat);
  var size = data.length;

  for (let row = h-1; row >=0; row--) {
    for (let col = w-1; col >=0 ; col--) {
      var index = col*4 + (row * w * 4);
      var index2 = (col*4 - ((row-(h-1)) * w * 4));
      data[index] = rtData[index2];
      data[index+1] = rtData[index2+1];
      data[index+2] = rtData[index2+2];
      data[index+3] = rtData[index2+3];
    }
  }

  return data;

}

function showTabPage(index) {

    $(".tabcontrol li").removeClass("active");
    $(".tabcontrol li[id=" + index + "]").addClass("active");

    $(".tabpage").each(function() {
        $(this).hide();
    });

    $(".tabpage[id=" + index + "]").show();

}

function formatDate(date) {
    var d = new Date(date),
        month = '' + (d.getMonth() + 1),
        day = '' + d.getDate(),
        year = d.getFullYear();

    if (month.length < 2)
        month = '0' + month;
    if (day.length < 2)
        day = '0' + day;

    return [year, month, day].join('-');
}

function onLoadedImageFile(filename, img) {

    var index = textures.length;
    var name = filename.split("/").pop();
    var newTexture = new THREE.Texture(img);

    var existingTexture = getTextureByName(name);

    if (existingTexture) {
        existingTexture.filename = filename;
        existingTexture.texture = newTexture;
        $('img[title="'+name+'"]').attr("src", img.src);
    } else {
        textures.push({name: name, filename: filename, texture: newTexture});
        $("#layersContainer .textureType").each(function(){
            $(this).append('<option value="' + name + '">' + filename + '</option>');
        });

        $("#selectBackgroundImage").each(function(){
            $(this).append('<option value="' + name + '">' + filename + '</option>');
        });

        $("#noTexturesUploaded").hide();

        var thumb = '<div class="textureThumbnailContainer"><img class="textureThumbnail" width="100" src="' + img.src + '" title="' + name + '" align="top" alt="" border="0"/></div>';
        $("#uploadedTexturesContainer").append(thumb);
    }

}

function loadImagesInSequence(files) {

    if (!files.length) {
        disableUI(false);
        document.getElementById("fileForm").reset();
        return;
    }

    var file = files.shift();

    var reader = new FileReader();
    reader.onload = function(e) {
        var img = new Image();
        img.onload = function() {
            onLoadedImageFile(file.name, this);
            loadImagesInSequence(files)
        };
        img.src = reader.result;
    };
    reader.readAsDataURL(file);

}


function uploadImage() {

    $('#imagefile').unbind();

    $('#imagefile').on("change", function(evt) {
        var target = evt.target || evt.srcElement;
        var files = [];
        disableUI(true);
        for(var i=0;i<target.files.length;i++) {
            files.push(target.files[i]);
        };
        loadImagesInSequence(files);
    });
    $('#imagefile').click();

    return false;

}

function createEmptyTexture(w, h) {

  var data = new Uint8Array(w*h*4);

  var texture = new THREE.DataTexture(data,w,h,THREE.RGBAFormat);
  texture.magFilter = THREE.NearestFilter;
  texture.minFilter = THREE.NearestFilter;
  var size = texture.image.width * texture.image.height;
  var data = texture.image.data;

  for (var i=0;i<size;i++) {
    var stride = i*4;
    data[stride] = 255;
    data[stride+1] = 255;
    data[stride+2] = 255;
    data[stride+3] = 255;
  }

  return texture;

}

function showPreview() {
    var player = new Player(
        "playerContainer",
        settings.get("viewportWidth"),
        settings.get("viewportHeight"),
        packedCanvas,
        $("#jsonOutputContainer textarea").val(),
        settings.get("dungeonDepth"),
        settings.get("dungeonWidth"),
        { r: settings.get("fogColor").r, g: settings.get("fogColor").g, b: settings.get("fogColor").b }
    );
}

function init() {

    if (isLoadingTexture) {
        return;
    }

    var viewportWidth = settings.get("viewportWidth");
    var viewportHeight = settings.get("viewportHeight");

    canvasContainer = $('#canvasContainer');
    canvasContainer.width(viewportWidth);
    canvasContainer.height(viewportHeight);

    imageCanvas = $('#imageCanvas')[0];
    imageCanvas.width = viewportWidth;
    imageCanvas.height = viewportHeight;
    ctx2D = imageCanvas.getContext('2d');
    ctx2D.imageSmoothingEnabled = false;

    if (!renderer) {
        renderer = new THREE.WebGLRenderer({alpha:true, premultipliedAlpha: false});
    }
    renderer.setPixelRatio(1);
    renderer.setSize(viewportWidth, viewportHeight);
    renderer.autoClear = false;

    bufferTexture = new THREE.WebGLRenderTarget(viewportWidth,viewportHeight, {format: THREE.RGBAFormat});

    whiteTexture = createEmptyTexture(256,256);

    if (!glcanvas) {
        glcanvas = renderer.domElement;
        $('#glcanvas').append(glcanvas);
    }

    if (!camera) {
        camera = new THREE.PerspectiveCamera(settings.get("cameraFOV"), viewportWidth / viewportHeight, 0.1, 1000);
    }
    camera.fov = settings.get("cameraFOV");
    camera.aspect = viewportWidth/viewportHeight;
    camera.updateProjectionMatrix();
    camera.position.set(settings.get("cameraOffsetX"), settings.get("cameraOffsetY") + 0.5, settings.get("cameraOffsetZ"));
    camera.rotation.set(settings.get("cameraTiltAngle"), 0, 0);

    if (!scene) {
        scene = new THREE.Scene();
    }

    scene.fog = null;

    // add light

    if (!light) {
        light = new THREE.PointLight(0xffffff, 1, 100);
        scene.add(light);
    }
    light.position.set(camera.position.x, camera.position.y, camera.position.z);

    /* create map */

    var map = [];

    var dungeonDepth = settings.get("dungeonDepth");
    var dungeonWidth = settings.get("dungeonWidth");

    for(var b=0;b<dungeonWidth; b++) {
        var arr = [];
        for(var a=0;a<=dungeonDepth; a++) {
            arr.push({x: 0-b, z: 0-a});
        }
        map.push(arr);
    }

    // clear previewObjectsGroup group

    if (!previewObjectsGroup) {
        previewObjectsGroup = new THREE.Group();
        scene.add(previewObjectsGroup);
    } else {
        for (var i = previewObjectsGroup.children.length - 1; i >= 0; i--) {
            previewObjectsGroup.remove(previewObjectsGroup.children[i]);
        }
    }

    // create the geometry

    squaresToGenerateList = [];

    map.forEach((row, j) => {
        row.forEach((square, i) => {
            // make sure the square camera is located on is ignored
            // if (!(square.x == 0 && square.z == 0)) {
                createPreviewObjects(square.x,square.z);
            // }
            squaresToGenerateList.push(square);
        });
    });



    //var material = new THREE.SpriteMaterial( { map: textures[0].texture, color: 0xffffff } );
    //var sprite = new THREE.Sprite( material );
    //sprite.scale.set(320/100,256/100,1);
    //scene.add( sprite );

    drawBackgroundImage();

}

function render() {
    renderer.setRenderTarget(null);
    // renderer.setClearColor(new THREE.Color(0.0,0.0,0.0));
    renderer.clear();
    renderer.render(scene, camera);
}

function drawBackgroundImage() {

    var textureName = $("#selectBackgroundImage").children("option:selected").val();
    var viewportWidth = settings.get("viewportWidth");
    var viewportHeight = settings.get("viewportHeight");

    if (textureName != "-1") {

        var existingTexture = getTextureByName(textureName);

        if (existingTexture) {
            ctx2D.drawImage(existingTexture.texture.image, 0, 0, viewportWidth,viewportHeight);
        } else {
            ctx2D.fillStyle = "black";
            ctx2D.fillRect(0, 0, viewportWidth,viewportHeight);
            ctx2D.clearRect(0, 0, viewportWidth,viewportHeight);
        }

    } else {
        ctx2D.fillStyle = "black";
        ctx2D.fillRect(0, 0, viewportWidth,viewportHeight);
        ctx2D.clearRect(0, 0, viewportWidth,viewportHeight);
    }

}

function setDefaultSettings() {

    settings.set("viewportWidth", 320);
    settings.set("viewportHeight", 256);
    settings.set("dungeonDepth", 5);
    settings.set("dungeonWidth", 4);

    settings.set("cameraOffsetX", 0.5);
    settings.set("cameraOffsetY", 0.005);
    settings.set("cameraOffsetZ", 0.026);
    settings.set("cameraTiltAngle", -0.15);
    settings.set("cameraFOV", 50);
    settings.set("ceilingOffsetY", -0.225);
    settings.set("tileWidth", 0.0);
    settings.set("cameraShiftZ", -0.115);
    settings.set("squareDepthScale", 1.0);
    settings.set("tilePadding", 2);
    settings.set("useFog", false);
    settings.set("fogStart", 0.0);
    settings.set("fogEnd", 3.5);
    settings.set("fogColor", {r: 0, g: 0, b: 0});

    settings.set("wireColor", {r: 255, g: 255, b: 255});

    // settings.set("cameraOffsetX", 0.5);
    // settings.set("cameraOffsetY", 0.005);
    // settings.set("cameraOffsetZ", -0.054);
    // settings.set("cameraTiltAngle", -0.15);
    // settings.set("cameraFOV", 50);
    // settings.set("ceilingOffsetY", -0.225);
    // settings.set("cameraShiftZ", -0.118);
    // settings.set("squareDepthScale", 1.0);
    // settings.set("tilePadding", 2);
    // settings.set("useFog", true);
    // settings.set("fogStart", 0.0);
    // settings.set("fogEnd", 6.0);
    // settings.set("fogColorR", 0);
    // settings.set("fogColorG", 0);
    // settings.set("fogColorB", 0);

    // settings.set("cameraOffsetX", 0.5);
    // settings.set("cameraOffsetY", -0.055);
    // settings.set("cameraOffsetZ", -0.355);
    // settings.set("cameraTiltAngle", -0.15);
    // settings.set("cameraFOV", 70);
    // settings.set("ceilingOffsetY", -0.225);
    // settings.set("cameraShiftZ", -0.117);
    // settings.set("squareDepthScale", 1.0);
    // settings.set("tilePadding", 2);
    // settings.set("useFog", false);
    // settings.set("fogStart", 0.0);
    // settings.set("fogEnd", 3.5);
    // settings.set("fogColor", {r: 0, g: 0, b: 0});

    settings.set("textureFiltering", false);

}

function resetValues() {

    $("#outputContainer").hide();
    $("#jsonOutputContainer").hide();
    $("#packedOutputContainer").empty();
    $("#btnPreview").hide();

    generatedData.layers.forEach((layer, i) => {
        layer.reset();
    });

    $(".slider").each(function() {
        var propName = $(this).attr("id");
        $(this).slider("option", "value", settings.get(propName));
        $(this).prev("input").val(settings.get(propName));
    })

    $('#checkboxUseShading').prop('checked', settings.get("useFog"));
    $('#checkboxTextureFiltering').prop('checked', settings.get("textureFiltering"));

    var wireColorR = settings.get("wireColor").r;
    var wireColorG = settings.get("wireColor").g;
    var wireColorB = settings.get("wireColor").b;
    var defaultColor = 'rgb('+wireColorR+','+wireColorG+','+wireColorB+')';
    $('.cp-alt-target1').css('background-color', defaultColor);
    $('.cp-alt-target1').colorpicker({
        color: defaultColor
    });

    var fogColorR = settings.get("fogColor").r;
    var fogColorG = settings.get("fogColor").g;
    var fogColorB = settings.get("fogColor").b;
    var defaultColor = 'rgb('+fogColorR+','+fogColorG+','+fogColorB+')';
    $('.cp-alt-target2').css('background-color', defaultColor);
    $('.cp-alt-target2').colorpicker({
        color: defaultColor
    });
}

function isBackgroundColor(r,g,b) {
    if (r != transparentBackgroundColor.r) { return false; }
    if (g != transparentBackgroundColor.g) { return false; }
    if (b != transparentBackgroundColor.b) { return false; }
    return true;
}

function grabDataRegion(x,y,w,h,width,data) {

  var result = new Uint8Array(w * h * 4);
  var a = 0;
  var b = 0;

  for (let row = y; row < y+h; row++) {
    for (let col = x; col < x+w ; col++) {
      var index = col*4 + (row * width * 4);
      var index2= a*4 + (b * w * 4);
      result[index2] = data[index];
      result[index2+1] = data[index+1];
      result[index2+2] = data[index+2];
      result[index2+3] = data[index+3];
      a++;
    }
    a = 0;
    b++;
  }

  return result;

}

function getRenderedPixels() {

    var data = getRenderTextureData();

    var x1 = null;
    var x2 = null;
    var y1 = null;
    var y2 = null;

    var viewportWidth = settings.get("viewportWidth");
    var viewportHeight = settings.get("viewportHeight");

    // find left side

    for(var x = 0; x < viewportWidth; x++) {
        for(var y = 0; y < viewportHeight; y++) {
            var red = data[((viewportWidth * y) + x) * 4];
            var green = data[((viewportWidth * y) + x) * 4 + 1];
            var blue = data[((viewportWidth * y) + x) * 4 + 2];
            var alpha = data[((viewportWidth * y) + x) * 4 + 3];
            if (alpha != 0) {
                if (x1 == null) {
                    x1 = x;
                }
            }
        }
    }

    // find right side

    for(var x = viewportWidth-1; x >= 0; x--) {
        for(var y = 0; y < viewportHeight; y++) {
            var red = data[((viewportWidth * y) + x) * 4];
            var green = data[((viewportWidth * y) + x) * 4 + 1];
            var blue = data[((viewportWidth * y) + x) * 4 + 2];
            var alpha = data[((viewportWidth * y) + x) * 4 + 3];
            if (alpha != 0) {
                if (x2 == null) {
                    x2 = x+1;
                }
            }
        }
    }

    // find top side

    for(var y = 0; y < viewportHeight; y++) {
        for(var x = 0; x < viewportWidth; x++) {
            var red = data[((viewportWidth * y) + x) * 4];
            var green = data[((viewportWidth * y) + x) * 4 + 1];
            var blue = data[((viewportWidth * y) + x) * 4 + 2];
            var alpha = data[((viewportWidth * y) + x) * 4 + 3];
            if (alpha != 0) {
                if (y1 == null) {
                    y1 = y;
                }
            }
        }
    }

    // find bottom side

    for(var y = viewportHeight-1; y >= 0; y--) {
        for(var x = viewportWidth-1; x >= 0; x--) {
            var red = data[((viewportWidth * y) + x) * 4];
            var green = data[((viewportWidth * y) + x) * 4 + 1];
            var blue = data[((viewportWidth * y) + x) * 4 + 2];
            var alpha = data[((viewportWidth * y) + x) * 4 + 3];
            if (alpha != 0) {
                if (y2 == null) {
                    y2 = y+1;
                }
            }
        }
    }

    if (x1 == null) { x1 = 0; }
    if (y1 == null) { y1 = 0; }

    if (x2 == null) { return null; }
    if (y2 == null) { return null; }


    data = grabDataRegion(x1, y1, x2-x1, y2-y1, viewportWidth, data)

    var imageData = ctx.createImageData(x2-x1, y2-y1);
    imageData.data.set(data);

    return {
        imageData: imageData,
        x: x1,
        y: y1,
        width: x2-x1,
        height: y2-y1
    }

}

function cropImageData(imageData) {

    var x1 = null;
    var x2 = null;
    var y1 = null;
    var y2 = null;

    var data = imageData.data;
    var w = imageData.width;
    var h = imageData.height;

    // find left side

    for(var x = 0; x < w; x++) {
        for(var y = 0; y < h; y++) {
            var r = data[((w * y) + x) * 4];
            var g = data[((w * y) + x) * 4 + 1];
            var b = data[((w * y) + x) * 4 + 2];
            if ((r != transparentBackgroundColor.r) && (g != transparentBackgroundColor.g) && (b != transparentBackgroundColor.b)) {
                if (x1 == null) {
                    x1 = x;
                }
            }
        }
    }

    // find right side

    for(var x = w-1; x >= 0; x--) {
        for(var y = 0; y < h; y++) {
            var r = data[((w * y) + x) * 4];
            var g = data[((w * y) + x) * 4 + 1];
            var b = data[((w * y) + x) * 4 + 2];
            if ((r != transparentBackgroundColor.r) && (g != transparentBackgroundColor.g) && (b != transparentBackgroundColor.b)) {
                if (x2 == null) {
                    x2 = x+1;
                }
            }
        }
    }

    // find top side

    for(var y = 0; y < h; y++) {
        for(var x = 0; x < w; x++) {
            var r = data[((w * y) + x) * 4];
            var g = data[((w * y) + x) * 4 + 1];
            var b = data[((w * y) + x) * 4 + 2];
            if ((r != transparentBackgroundColor.r) && (g != transparentBackgroundColor.g) && (b != transparentBackgroundColor.b)) {
                if (y1 == null) {
                    y1 = y;
                }
            }
        }
    }

    // find bottom side

    for(var y = h-1; y >= 0; y--) {
        for(var x = w; x >= 0; x--) {
            var r = data[((w * y) + x) * 4];
            var g = data[((w * y) + x) * 4 + 1];
            var b = data[((w * y) + x) * 4 + 2];
            var a = data[((w * y) + x) * 4 + 3];
            if ((r != transparentBackgroundColor.r) && (g != transparentBackgroundColor.g) && (b != transparentBackgroundColor.b)) {
                if (y2 == null) {
                    y2 = y+1;
                }
            }
        }
    }

    if (x1 == null) { x1 = 0; }
    if (y1 == null) { y1 = 0; }

    if (x2 == null) { x2 = w; }
    if (y2 == null) { y2 = h; }

    var data = grabDataRegion(x1, y1, x2-x1, y2-y1, w, data)

    var imageData = ctx.createImageData(x2-x1, y2-y1);
    imageData.data.set(data);

    return {
        imageData: imageData,
        x: x1,
        y: y1,
        width: (x2-x1),
        height: (y2-y1)
    }

}

function createImage(data) {
    return createImageBitmap(data);
}

function getScreenPosition(position) {

    var vector = new THREE.Vector3(position.x, position.y, position.z);

    vector.project(camera);

    var viewportWidth = settings.get("viewportWidth");
    var viewportHeight = settings.get("viewportHeight");

    vector.x = Math.round((   vector.x + 1 ) * viewportWidth / 2);
    vector.y = Math.round(( - vector.y + 1 ) * viewportHeight / 2);

    return vector;
}

function _debugShowImage(imageData) {

    var tmpCanvas = document.createElement("canvas");
    tmpCanvas.width = imageData.width;
    tmpCanvas.height = imageData.height;
    var tmpCtx = tmpCanvas.getContext("2d");
    tmpCtx.putImageData(imageData, 0, 0);

    document.body.appendChild(tmpCanvas);

}

function getTextureByName(name) {
    for(var i = 0; i < textures.length; i++) {
        if (textures[i].name == name) {
            return textures[i];
        }
    }

    return null;

}

function renderFirstPass(texture) {
    setPass(1, texture);
    return getRenderedPixels();
}

function renderSecondPass(fullwidth, texture) {

    var tilePadding = settings.get("tilePadding");

    setPass(2, texture);

    var region = getRenderedPixels();

    if (region) {

        var croppedRegion = cropImageData(region.imageData);

        if (croppedRegion) {

            // _debugShowImage(croppedRegion.imageData);

            var rw = croppedRegion.width + tilePadding;
            var rh = croppedRegion.height + tilePadding;
            var tmpCanvas = document.createElement("canvas");
            tmpCanvas.width = rw;
            tmpCanvas.height = rh;
            var tmpCtx = tmpCanvas.getContext("2d");
            tmpCtx.putImageData(croppedRegion.imageData, 0, 0);

            return {
                canvas: tmpCanvas,
                x: region.x+croppedRegion.x,
                y: region.y+croppedRegion.y,
                fullWidth: fullwidth
            }

        }

    }

    return null;

}

function packImage(layerIndex, x, z, type, data) {

    totalTilesToPack++;

    var img = new Image();
    img.src = data.canvas.toDataURL("image/png");
    img.layerIndex = layerIndex;
    img.type = type;
    img.squareX = x;
    img.squareZ = z;
    img.screenPosX = data.x;
    img.screenPosY = data.y;
    img.fullWidth = data.fullWidth;
    img.onload = function() {
        var node = atlas.pack(img);
        if (node === false) {
            atlas = atlas.expand(img);
        }
        currentlyPackedTilesNumber++;
    };

}

function renderFloor(layer) {

    var dungeonDepth = settings.get("dungeonDepth");
    var texture = getTextureByName(layer.textureName);

    squaresToGenerateList.forEach((square, i) => {
        createSingleFloorObject(layer);
        generateObjectGroup.position.set(square.x,0,square.z-1);
        if (region = renderFirstPass(texture)) {
            createSingleFloorObject(layer);
            generateObjectGroup.position.set(square.x,0,square.z-1);
            if (result = renderSecondPass(region.width, texture)) {
                packImage(layer.index, square.x, square.z, "floor", result);
            }
        }
    });

    clearGeneratedObjects();

}

function renderCeiling(layer) {

    var dungeonDepth = settings.get("dungeonDepth");
    var texture = getTextureByName(layer.textureName);

    squaresToGenerateList.forEach((square, i) => {
        createSingleCeilingObject(layer);
        generateObjectGroup.position.set(square.x,0,square.z-1);
        if (region = renderFirstPass(texture)) {
            createSingleCeilingObject(layer);
            generateObjectGroup.position.set(square.x,0,square.z-1);
            if (result = renderSecondPass(region.width, texture)) {
                packImage(layer.index, square.x, square.z, "ceiling", result);
            }
        }
    });

    clearGeneratedObjects();

}

function renderWalls(layer) {

    var dungeonDepth = settings.get("dungeonDepth");
    var texture = getTextureByName(layer.textureName);

    squaresToGenerateList.forEach((square, i) => {
        if (i <= dungeonDepth) { // only render one set of front walls since they all are the same on each side.
            createObject(layer, WALL_FRONT);
            generateObjectGroup.position.set(square.x,0,square.z);
            if (region = renderFirstPass(texture))
                if (result = renderSecondPass(region.width, texture))
                    packImage(layer.index, square.x, square.z, "front", result);
        }
    });

    clearGeneratedObjects();

    squaresToGenerateList.forEach((square, i) => {
        createObject(layer, WALL_SIDE);
        generateObjectGroup.position.set(square.x,0,square.z);
        if (region = renderFirstPass(texture))
            if (result = renderSecondPass(region.width, texture))
                packImage(layer.index, square.x-1, square.z, "side", result);
    });

    clearGeneratedObjects();

}

function renderDecals(layer) {

    var dungeonDepth = settings.get("dungeonDepth");
    var texture = getTextureByName(layer.textureName);

    squaresToGenerateList.forEach((square, i) => {
        if (i <= dungeonDepth) { // only render one set of front walls since they all are the same on each side.
            createObject(layer, WALL_FRONT);
            generateObjectGroup.position.set(square.x,0,square.z);
            if (region = renderFirstPass(texture)) {
                createDecalObject(layer, WALL_FRONT);
                if (result = renderSecondPass(region.width, texture)) {
                    packImage(layer.index, square.x, square.z, "front", result);
                }
            }
        }
    });

    clearGeneratedObjects();

    squaresToGenerateList.forEach((square, i) => {
        createObject(layer, WALL_SIDE);
        generateObjectGroup.position.set(square.x,0,square.z);
        if (region = renderFirstPass(texture)) {
            createDecalObject(layer, WALL_SIDE);
            if (result = renderSecondPass(region.width, texture)) {
                packImage(layer.index, square.x-1, square.z, "side", result);
            }
        }
    });

    clearGeneratedObjects();

}

function renderObjects(layer) {

    var dungeonDepth = settings.get("dungeonDepth");
    var texture = getTextureByName(layer.textureName);

    squaresToGenerateList.forEach((square, i) => {
        if (i < dungeonDepth) { // only render one set of front walls since they all are the same on each side.
            createObjectObject1(layer, square.z);
            generateObjectGroup.position.set(square.x,0,square.z);
            if (region = renderFirstPass(texture)) {
                createObjectObject2(layer, square.z);
                if (result = renderSecondPass(region.width, texture)) {
                    packImage(layer.index, square.x, square.z, "object", result);
                }
            }
        }
    });

    clearGeneratedObjects();

}

function disableUI(value) {

    if (value) {
        $('#atlasgenerator').find(':input').prop('disabled', true);
        $("#outputContainer").hide();
        $("#jsonOutputContainer").hide();
        $("#packedOutputContainer").empty();
        $("#btnPreview").hide();
    } else {
        $("#packedCanvas").show();
        $("#outputContainer").show();
        $('#atlasgenerator').find(':input').prop('disabled', false);
        $("#jsonOutputContainer").show();
        $("#btnPreview").show();
    }

}

function generate() {

    if (generatedData.layers.length == 0) {
        alert("You need at least one layer in order to generate atlas.");
        return false;
    }

    disableUI(true);

    var viewportWidth = settings.get("viewportWidth");
    var viewportHeight = settings.get("viewportHeight");

    packedCanvas = document.createElement("canvas");
    packedCanvas.style.backgroundColor = 'fuchsia';
    packedCanvas.style.backgroundColor = 'black';
    packedCanvas.style.border = '1px dashed #555';
    $("#packedOutputContainer").append(packedCanvas);
    packedCanvas.width = 16;
    packedCanvas.height = 16;
    atlas = window.atlaspack(packedCanvas);

    $("#packedCanvas").hide();

    var fogColor = settings.get("fogColor");
    scene.fog = new THREE.Fog(new THREE.Color(fogColor.r/255, fogColor.g/255, fogColor.b/255), settings.get("fogStart"), settings.get("fogEnd"));

    previewObjectsGroup.visible = false;
    var canvasContainer2D = $('#canvasContainer2D');
    canvasContainer2D.width(viewportWidth);
    canvasContainer2D.height(viewportHeight);
    canvas.width = viewportWidth;
    canvas.height = viewportHeight;

    if (!generateObjectGroup) {
        generateObjectGroup = new THREE.Group();
        scene.add(generateObjectGroup);
    }

    generatedData.version = "0.0.1";
    generatedData.generated = formatDate(new Date());

    for(var i=0;i<generatedData.layers.length;i++) {

        generatedData.layers[i].reset();

        if (generatedData.layers[i].on && generatedData.layers[i].type == LAYER_TYPE_WALLS) {
            renderWalls(generatedData.layers[i]);
        }
        if (generatedData.layers[i].on && generatedData.layers[i].type == LAYER_TYPE_DECAL) {
            renderDecals(generatedData.layers[i]);
        }

        if (generatedData.layers[i].on && (generatedData.layers[i].type == LAYER_TYPE_OBJECT)) {
            renderObjects(generatedData.layers[i]);
        }

        if (generatedData.layers[i].on && (generatedData.layers[i].type == LAYER_TYPE_FLOOR)) {
            renderFloor(generatedData.layers[i]);
        }

        if (generatedData.layers[i].on && (generatedData.layers[i].type == LAYER_TYPE_CEILING)) {
            renderCeiling(generatedData.layers[i]);
        }

    }

    previewObjectsGroup.visible = true;

    scene.fog = null;

    disableUI(false);

}

function setPass(index, tex) {

    switch(index) {
        case 1: {

            var material = new THREE.MeshBasicMaterial({
                color: 0xffffff,
                side: THREE.DoubleSide,
                map: whiteTexture
            });

        } break;

        case 2: {

            if (tex) {
                tex.texture.wrapS = THREE.ClampToEdgeWrapping;
                tex.texture.wrapT = THREE.ClampToEdgeWrapping;
                if (settings.get("textureFiltering") == true) {
                    tex.texture.minFilter = THREE.LinearFilter;
                    tex.texture.magFilter = THREE.LinearFilter;
                    tex.texture.anisotropy = 0;
                } else {
                    tex.texture.minFilter = THREE.NearestFilter;
                    tex.texture.magFilter = THREE.NearestFilter;
                    tex.texture.anisotropy = 0;
                }
                tex.texture.generateMipmaps = false;
                tex.texture.needsUpdate = true;
            }

            var material = new THREE.MeshBasicMaterial({
                color: 0xffffff,
                side: THREE.DoubleSide,
                fog: settings.get("useFog"),
                map: tex ? tex.texture : whiteTexture
            });

        } break;
    }

    if (generateObjectGroup) {
        for (var i = 0; i < generateObjectGroup.children.length; i++) {
            generateObjectGroup.children[i].material = material;
        }
    }

    renderer.setRenderTarget(bufferTexture,0,0);
    // renderer.setClearColor(new THREE.Color(0.0,0.0,0.0), 0);
    renderer.clear();
    renderer.render(scene, camera);

}

function clearGeneratedObjects() {

    for (var i = generateObjectGroup.children.length - 1; i >= 0; i--) {
        generateObjectGroup.remove(generateObjectGroup.children[i]);
    }

}

function createObject(layer, type) {

    clearGeneratedObjects();

    var ceilingOffsetY = settings.get("ceilingOffsetY");
    var tileWidth = settings.get("tileWidth");
    var cameraShiftZ = settings.get("cameraShiftZ");

    switch(type) {
        case WALL_FRONT: {
            var matrix = new THREE.Matrix4();
            matrix.set(1, 0, 0, 0, 0, 1+ceilingOffsetY, 0, 0, 0, cameraShiftZ, 1, 0, 0, 0, 0, 1);
            var planeGeom = new THREE.PlaneGeometry(1, 1, 2);
            var mesh = new THREE.Mesh(planeGeom);
            mesh.position.set(0.5,0.5+(ceilingOffsetY*0.5),cameraShiftZ*0.5);
            planeGeom.applyMatrix4(matrix);
            generateObjectGroup.add(mesh);
        } break;
        case WALL_SIDE: {
            var matrix = new THREE.Matrix4();
            matrix.set(1, -cameraShiftZ, 0, 0, 0, 1+ceilingOffsetY, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
            var planeGeom = new THREE.PlaneGeometry(1, 1, 2);
            planeGeom.applyMatrix4(matrix);
            var mesh = new THREE.Mesh(planeGeom);
            mesh.rotation.set(0,THREE.Math.degToRad(90),0);
            mesh.position.set(0,(0.5)+ceilingOffsetY*0.5,-0.5+(cameraShiftZ*0.5));
            generateObjectGroup.add(mesh);
        } break;
    }

}

function createSingleFloorObject(layer) {

    clearGeneratedObjects();

    var cameraShiftZ = settings.get("cameraShiftZ");

    var planeGeom = new THREE.PlaneGeometry(1, 1, 2);
    var mesh = new THREE.Mesh(planeGeom);
    mesh.position.set(0.5,0.0,0.5);
    mesh.rotation.set(THREE.Math.degToRad(90),0,0);
    generateObjectGroup.add(mesh);

}

function createSingleCeilingObject(layer) {

    clearGeneratedObjects();

    var ceilingOffsetY = settings.get("ceilingOffsetY");
    var tileWidth = settings.get("tileWidth");
    var cameraShiftZ = settings.get("cameraShiftZ");

    var matrix = new THREE.Matrix4();
    var planeGeom = new THREE.PlaneGeometry(1, 1, 2);
    var mesh = new THREE.Mesh(planeGeom);
    mesh.position.set(0.5,1.0+ceilingOffsetY,(0.5)+cameraShiftZ);
    mesh.rotation.set(THREE.Math.degToRad(90),0,0);
    generateObjectGroup.add(mesh);

}

function createDecalObject(layer, type) {

    clearGeneratedObjects();

    var ceilingOffsetY = settings.get("ceilingOffsetY");
    var tileWidth = settings.get("tileWidth");
    var cameraShiftZ = settings.get("cameraShiftZ");

    switch(type) {
        case WALL_FRONT: {
            var matrix = new THREE.Matrix4();
            matrix.set(1, 0, 0, 0, 0, 1+ceilingOffsetY, 0, 0, 0, cameraShiftZ, 1, 0, 0, 0, 0, 1);
            var planeGeom = new THREE.PlaneGeometry(layer.scale.x, layer.scale.y, 2);
            planeGeom.applyMatrix4(matrix);
            var mesh = new THREE.Mesh(planeGeom);
            mesh.position.set(0.5,(0.5+(ceilingOffsetY*0.5))+layer.offset.y,cameraShiftZ*0.5);
            generateObjectGroup.add(mesh);
        } break;
        case WALL_SIDE: {
            var matrix = new THREE.Matrix4();
            matrix.set(1, -cameraShiftZ, 0, 0, 0, 1+ceilingOffsetY, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
            var planeGeom = new THREE.PlaneGeometry(layer.scale.x, layer.scale.y, 2);
            planeGeom.applyMatrix4(matrix);
            var mesh = new THREE.Mesh(planeGeom);
            mesh.rotation.set(0,THREE.Math.degToRad(90),0);
            mesh.position.set(0,((0.5)+ceilingOffsetY*0.5)+layer.offset.y,-0.5+(cameraShiftZ*0.5));
            generateObjectGroup.add(mesh);
        } break;
    }

}

function createObjectObject1(layer, z) {

    clearGeneratedObjects();

    var ceilingOffsetY = settings.get("ceilingOffsetY");
    var tileWidth = settings.get("tileWidth");
    var cameraShiftZ = settings.get("cameraShiftZ");

    var matrix = new THREE.Matrix4();
    matrix.set(1, 0, 0, 0, 0, 1+ceilingOffsetY, 0, 0, 0, cameraShiftZ, 1, 0, 0, 0, 0, 1);
    var planeGeom = new THREE.PlaneGeometry(1, 1, 2);
    var mesh = new THREE.Mesh(planeGeom);

    var zz = (z == 0) ? -0.6 : -0.15;

    mesh.position.set(0.5,0.5+(ceilingOffsetY*0.5), zz + (cameraShiftZ*0.5));
    planeGeom.applyMatrix4(matrix);
    generateObjectGroup.add(mesh);

}

function createObjectObject2(layer, z) {

    clearGeneratedObjects();

    var ceilingOffsetY = settings.get("ceilingOffsetY");
    var tileWidth = settings.get("tileWidth");
    var cameraShiftZ = settings.get("cameraShiftZ");

    var matrix = new THREE.Matrix4();
    matrix.set(1, 0, 0, 0, 0, 1+ceilingOffsetY, 0, 0, 0, cameraShiftZ, 1, 0, 0, 0, 0, 1);
    var planeGeom = new THREE.PlaneGeometry(layer.scale.x, layer.scale.y, 2);
    planeGeom.applyMatrix4(matrix);
    var mesh = new THREE.Mesh(planeGeom);

    var zz = (z == 0) ? -0.6 : -0.15;

    mesh.position.set(0.5,(0.5+(ceilingOffsetY*0.5)) + layer.offset.y, zz + (cameraShiftZ*0.5));
    generateObjectGroup.add(mesh);

}

function createPreviewObjects(x, z) {

    var ceilingOffsetY = settings.get("ceilingOffsetY");
    var tileWidth = settings.get("tileWidth");
    var cameraShiftZ = settings.get("cameraShiftZ");

    var matrix = new THREE.Matrix4();
    matrix.set(1+tileWidth, 0, 0, 0, 0, 1+ceilingOffsetY, 0, 0, 0, cameraShiftZ, 1, 0, 0, 0, 0, 1);

    var geometry = new THREE.BoxGeometry(1,1,1);
    var wireframe = new THREE.EdgesGeometry(geometry);
    wireframe.applyMatrix4(matrix);

    var wireColor = settings.get("wireColor");
    var color = new THREE.Color(wireColor.r/255, wireColor.g/255, wireColor.b/255, 1.0);

    var mat = new THREE.LineBasicMaterial({color: color, linewidth: 1});
    var cube = new THREE.LineSegments(wireframe, mat);

    //x -= tileWidth*0.5;

    cube.position.set(x+0.5,(0.5)+ceilingOffsetY*0.5,(((z-0.5))+cameraShiftZ*0.5));
    previewObjectsGroup.add(cube);

}

function getLayerByIndex(index) {
    for(var i=0;i<generatedData.layers.length;i++) {
        if (generatedData.layers[i].index == index) {
            return generatedData.layers[i];
        }
    }
    return null;
}

function getLayerIndexById(id) {
    for(var i=0;i<generatedData.layers.length;i++) {
        if (generatedData.layers[i].id == id) {
            return i;
        }
    }
    return null;
}

function atlasCallback(layerIndex, type, fullWidth, tilePos, screenPos, rect) {

    var tile = {
        type: type,
        flipped: false,
        tile: {
            x: tilePos.x,
            z: tilePos.z
        },
        screen: {
            x: screenPos.x,
            y: screenPos.y
        },
        coords: {
            x: rect.x,
            y: rect.y,
            w: rect.w - settings.get("tilePadding"),
            h: rect.h - settings.get("tilePadding"),
            fullWidth: fullWidth
        }
    };

    var layer = getLayerByIndex(layerIndex);

    layer.addTile(tile);

    if (type == "side") {

        var tile = {
            type: type,
            flipped: true,
            tile: {
                x: Math.abs(tilePos.x),
                z: tilePos.z
            },
            screen: {
                x: settings.get("viewportWidth") - screenPos.x,
                y: screenPos.y
            },
            coords: {
                x: rect.x,
                y: rect.y,
                w: rect.w - settings.get("tilePadding"),
                h: rect.h - settings.get("tilePadding"),
                fullWidth: settings.get("tilePadding")
            }
        };

        layer.addTile(tile);

    }

    if ((type == "floor" || type == "ceiling") && tilePos.x < 0) {

        var tile = {
            type: type,
            flipped: true,
            tile: {
                x: Math.abs(tilePos.x),
                z: tilePos.z
            },
            screen: {
                x: settings.get("viewportWidth") - screenPos.x,
                y: screenPos.y
            },
            coords: {
                x: rect.x,
                y: rect.y,
                w: rect.w - settings.get("tilePadding"),
                h: rect.h - settings.get("tilePadding"),
                fullWidth: settings.get("tilePadding")
            }
        };

        layer.addTile(tile);

    }

    if (currentlyPackedTilesNumber == totalTilesToPack-1) {

        var json_string = prepareGeneratedJSON();

        $("#jsonOutputContainer textarea").val(json_string);
    }

}

function deleteExcludedLayer(layers, id) {
    for(var i=0;i<layers.length;i++) {
        if (layers[i].id == id) {
            layers.splice(i,1);
        }
    }
    return layers;
}

$(document).ready(function() {

    canvas = document.getElementById('outputCanvas');
    ctx = canvas.getContext('2d');

    vertexShader = document.getElementById('vertexShader').textContent;
    fragmentShader = document.getElementById('fragmentShader').textContent;

    packedCanvas = document.getElementById('packedCanvas');

    $("#atlasgenerator").hide();

    loadingManager = new THREE.LoadingManager(function() {
        setDefaultSettings();
        initUI();
        resetValues();
        showTabPage("main");
        init();
        $("#loadingTexturesPleaseWait").hide();
        $("#atlasgenerator").show();
        mainloop();
    });

    textureFilenames.forEach((element, i) => {
        var loader = new THREE.TextureLoader(loadingManager);
        var name = element.filename.split("/").pop();

        loader.load(element.filename + "?" + new Date().getTime(), function(texture) {
            textures.push({name: element.name, filename: element.filename, texture: texture});
            var thumb = '<div class="textureThumbnailContainer"><img class="textureThumbnail" width="100" src="' + element.filename + '" title="' + element.name + '" align="top" alt="" border="0"/></div>';
            $("#builtinTexturesContainer").append(thumb);
        });

    });


    $("#btnReset").click(function(){
        setDefaultSettings();
        resetValues();
        init();
    });

    $("#btnAddLayer").click(function() {
        addLayer();
    });

    $("#btnPreview").click(function(){
        showPreview();
    });

    $("#btnUpload").click(function(){
        uploadImage();
    });

    $("#btnCopyToClipboard").click(function(){
        copyJSONToClipboard();
    });

    $(".tabcontrol div").each(function() {
        $(this).click(function() {
            showTabPage($(this).parent().attr("id"));
        })
    });

    $("#btnGenerate").click(function() {
        generate();
    });

    $("#btnLoadPreset").click(function() {
        loadPreset();
    });

    $("#btnSavePreset").click(function() {
        savePreset();
    });

    $("#btnSaveJSON").click(function() {
        saveJSON();
    });

})
