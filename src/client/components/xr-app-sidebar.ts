import { html, css, LitElement } from 'lit';
import { customElement } from 'lit/decorators.js';

import './xr-album-list'

@customElement('xr-app-sidebar')
export class XrAppSidebar extends LitElement {
  static styles = css`
    :host {
      width: 300px;
      padding: 8px 0;
      display: flex;
      flex-direction: column;
      background-color: #111;
      overflow-y: scroll;
    }
  `;

  override render() {
    return html`<xr-album-list path="/"></xr-album-list>`;
  }
}
