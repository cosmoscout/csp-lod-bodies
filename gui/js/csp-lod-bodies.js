/* global IApi, CosmoScout, $ */

/**
 * LOD Bodies Api
 */
class LODBodyApi extends IApi {
  /**
   * @inheritDoc
   */
  name = 'lodBody';

  /**
   * @inheritDoc
   */
  init() {
    CosmoScout.gui.initSlider('set_terrain_height', 1.0, 20.0, 0.1, [1]);
    CosmoScout.gui.initSlider('set_height_range', -12.0, 21.0, 0.1, [-8, 12]);
    CosmoScout.gui.initSlider('set_slope_range', 0.0, 90.0, 1.0, [0, 45]);
    CosmoScout.gui.initSlider('set_terrain_lod', 10.0, 50.0, 0.1, [15]);
    CosmoScout.gui.initSlider('set_texture_gamma', 0.1, 3.0, 0.01, [1.0]);

    const terrainLod = document.getElementById('set_terrain_lod');

    if (terrainLod === null) {
      return;
    }

    document.getElementById('set_enable_auto_terrain_lod').addEventListener('change', (event) => {
      if (event.target.checked) {
        terrainLod.classList.add('unresponsive');
      } else {
        terrainLod.classList.remove('unresponsive');
      }
    });
  }

  /**
   * Sets an elevation data copyright tooltip
   * TODO Remove jQuery
   *
   * @param copyright {string}
   */
  // eslint-disable-next-line class-methods-use-this
  setElevationDataCopyright(copyright) {
    $('#img-data-copyright').tooltip({ title: `© ${copyright}`, placement: 'top' });
  }

  /**
   * Sets a map data copyright tooltip
   * TODO Remove jQuery
   *
   * @param copyright {string}
   */
  // eslint-disable-next-line class-methods-use-this
  setMapDataCopyright(copyright) {
    $('#dem-data-copyright').tooltip({ title: `© ${copyright}`, placement: 'bottom' });
  }
}

// Init Class on file load
(() => {
  CosmoScout.init(LODBodyApi);
})();
