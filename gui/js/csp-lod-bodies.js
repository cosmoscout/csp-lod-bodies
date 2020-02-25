/* global IApi, CosmoScout, $ */

/**
 * LOD Bodies Api
 */
class LODBodiesApi extends IApi {
  /**
   * @inheritDoc
   */
  name = 'lodBodies';

  /**
   * @inheritDoc
   */
  init() {
    CosmoScout.gui.initSlider('lodBodies.setTerrainHeight', 1.0, 20.0, 0.1, [1]);
    CosmoScout.gui.initSlider('lodBodies.setHeightRange', -12.0, 21.0, 0.1, [-8, 12]);
    CosmoScout.gui.initSlider('lodBodies.setSlopeRange', 0.0, 90.0, 1.0, [0, 45]);
    CosmoScout.gui.initSlider('lodBodies.setTerrainLod', 10.0, 50.0, 0.1, [15]);
    CosmoScout.gui.initSlider('lodBodies.setTextureGamma', 0.1, 3.0, 0.01, [1.0]);

    const terrainLod = document.getElementById('lodBodies.setTerrainLod');

    if (terrainLod === null) {
      return;
    }

    document.getElementById('lodBodies.setEnableAutoTerrainLod').addEventListener('change', (event) => {
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
  CosmoScout.init(LODBodiesApi);
})();
