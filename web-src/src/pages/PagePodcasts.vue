<template>
  <div>
    <content-with-heading v-if="new_episodes.items.length > 0">
      <template slot="heading-left">
        <p class="title is-4">New episodes</p>
      </template>
      <template slot="heading-right">
      <div class="buttons is-centered">
        <a class="button is-small" @click="mark_all_played">
          <span class="icon">
            <i class="mdi mdi-pencil"></i>
          </span>
          <span>Mark All Played</span>
        </a>
      </div>
    </template>
    <template slot="content">
        <list-item-track v-for="track in new_episodes.items" :key="track.id" :track="track" @click="play_track(track)">
          <template slot="progress">
            <range-slider
              class="track-progress"
              min="0"
              :max="track.length_ms"
              step="1"
              :disabled="true"
              :value="track.seek_ms" >
            </range-slider>
          </template>
          <template slot="actions">
            <a @click="open_track_dialog(track)">
              <span class="icon has-text-dark"><i class="mdi mdi-dots-vertical mdi-18px"></i></span>
            </a>
          </template>
        </list-item-track>
        <modal-dialog-track :show="show_track_details_modal" :track="selected_track" @close="show_track_details_modal = false" @play-count-changed="reload_new_episodes" />
      </template>
    </content-with-heading>

    <content-with-heading>
      <template slot="heading-left">
        <p class="title is-4">Podcasts</p>
        <p class="heading">{{ albums.total }} podcasts</p>
      </template>
      <template slot="heading-right">
        <div class="buttons is-centered">
          <a class="button is-small" @click="open_add_podcast_dialog">
            <span class="icon">
              <i class="mdi mdi-rss"></i>
            </span>
            <span>Add Podcast</span>
          </a>
        </div>
      </template>
      <template slot="content">
        <list-albums :albums="albums.items"
            @play-count-changed="reload_new_episodes()"
            @podcast-deleted="reload_podcasts()">
        </list-albums>
        <modal-dialog-add-rss
            :show="show_url_modal"
            @close="show_url_modal = false"
            @podcast-added="reload_podcasts()" />
      </template>
    </content-with-heading>
  </div>
</template>

<script>
import { LoadDataBeforeEnterMixin } from './mixin'
import ContentWithHeading from '@/templates/ContentWithHeading'
import ListItemTrack from '@/components/ListItemTrack'
import ListAlbums from '@/components/ListAlbums'
import ModalDialogTrack from '@/components/ModalDialogTrack'
import ModalDialogAddRss from '@/components/ModalDialogAddRss'
import RangeSlider from 'vue-range-slider'
import webapi from '@/webapi'

const albumsData = {
  load: function (to) {
    return Promise.all([
      webapi.library_albums('podcast'),
      webapi.library_podcasts_new_episodes()
    ])
  },

  set: function (vm, response) {
    vm.albums = response[0].data
    vm.new_episodes = response[1].data.tracks
  }
}

export default {
  name: 'PagePodcasts',
  mixins: [LoadDataBeforeEnterMixin(albumsData)],
  components: { ContentWithHeading, ListItemTrack, ListAlbums, ModalDialogTrack, ModalDialogAddRss, RangeSlider },

  data () {
    return {
      albums: { items: [] },
      new_episodes: { items: [] },

      show_url_modal: false,

      show_track_details_modal: false,
      selected_track: {}
    }
  },

  methods: {
    play_track: function (track) {
      webapi.player_play_uri(track.uri, false)
    },

    open_track_dialog: function (track) {
      this.selected_track = track
      this.show_track_details_modal = true
    },

    mark_all_played: function () {
      this.new_episodes.items.forEach(ep => {
        webapi.library_track_update(ep.id, { play_count: 'increment' })
      })
      this.new_episodes.items = { }
    },

    open_add_podcast_dialog: function (item) {
      this.show_url_modal = true
    },

    reload_new_episodes: function () {
      webapi.library_podcasts_new_episodes().then(({ data }) => {
        this.new_episodes = data.tracks
      })
    },

    reload_podcasts: function () {
      webapi.library_albums('podcast').then(({ data }) => {
        this.albums = data
        this.reload_new_episodes()
      })
    }
  }
}
</script>

<style>
</style>
