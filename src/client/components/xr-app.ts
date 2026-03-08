import { html, css, LitElement } from 'lit';
import { customElement, state } from 'lit/decorators.js';

import './xr-app-sidebar';
import './xr-app-content';
import * as types from '../modules/client_types';

@customElement('xr-app')
export class XrApp extends LitElement {
  static styles = css`
    :host {
      width: 100vw;
      height: 100vh;
      display: flex;
      flex-direction: row;
    }
    xr-app-sidebar {
      flex-shrink: 0;
    }
    xr-app-content {
      flex-grow: 1;
    }
  `;

  @state() private album?: types.AlbumResponse;

  override render() {
    const onSelected = (e: CustomEvent<types.AlbumResponse>) => {
      this.album = e.detail;
    };
    return html`
      <xr-app-sidebar @selected=${onSelected}></xr-app-sidebar>
      <xr-app-content .album=${this.album}></xr-app-content>
    `;
  }
}
