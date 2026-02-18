import { html, css, LitElement } from 'lit';
import { customElement, state } from 'lit/decorators.js';

import './xr-app-sidebar';
import './xr-app-viewer';
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
    xr-app-viewer {
      flex-grow: 1;
    }
  `;

  @state() private selectedMedia?: types.SelectedMedia;

  override render() {
    const onSelected = (e: CustomEvent<types.SelectedMedia>) => {
      this.selectedMedia = e.detail;
    };
    return html`
      <xr-app-sidebar @media-selected=${onSelected}></xr-app-sidebar>
      <xr-app-viewer .media=${this.selectedMedia}></xr-app-viewer>
    `;
  }
}
